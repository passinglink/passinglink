#include "output/usb/ps4/auth.h"

#include <zephyr.h>

#include <kernel.h>
#include <logging/log.h>

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4_AUTH)
#include <mbedtls/error.h>
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>

#include "ds4.inl.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(PS4Auth);

extern "C" void dump_allocator_hwm();

#if defined(CONFIG_PASSINGLINK_CHECK_MAIN_STACK_HWM)
extern "C" K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
#endif

namespace ps4 {

#if HAVE_DS4_KEY
static mbedtls_rsa_context* ds4_key = const_cast<mbedtls_rsa_context*>(&__ds4_key);
static const u8_t* ds4_key_e = reinterpret_cast<const u8_t*>(__ds4_key_e);
static const u8_t* ds4_key_n = reinterpret_cast<const u8_t*>(__ds4_key_n);
static const u8_t* ds4_serial = reinterpret_cast<const u8_t*>(__ds4_serial);
static const u8_t* ds4_signature = reinterpret_cast<const u8_t*>(__ds4_signature);
#else
static mbedtls_rsa_context* ds4_key = nullptr;
static const u8_t* ds4_key_e = nullptr;
static const u8_t* ds4_key_n = nullptr;
static const u8_t* ds4_serial = nullptr;
static const u8_t* ds4_signature = nullptr;
#endif

static void sign_nonce(struct k_work*);

static atomic_u32<AuthState> auth_state;
static u8_t nonce[256];
static u8_t nonce_signature[256];

K_WORK_DEFINE(k_work_sign, sign_nonce);

AuthState get_auth_state() {
  return auth_state.load();
}

static void sign_nonce(struct k_work*) {
  LOG_INF("sign_nonce: started");

  AuthState current_state = auth_state.load();
  if (current_state.type != AuthStateType::WaitingToSign) {
    LOG_ERR("sign_nonce: unexpected auth state: %d", static_cast<int>(current_state.type));
    return;
  }

  AuthState new_state = current_state;
  new_state.type = AuthStateType::Signing;
  if (!auth_state.cas(current_state, new_state)) {
    LOG_ERR("sign_nonce: failed to exchange initial auth state");
    return;
  }

  auto rng = [](void*, unsigned char* p, size_t len) {
    memset(p, 0, len);
    return 0;
  };

  u8_t hashed_nonce[32];
  if (mbedtls_sha256_ret(nonce, sizeof(nonce), hashed_nonce, 0) != 0) {
    LOG_ERR("sign_nonce: failed to hash nonce");
    return;
  }

  int rc =
      mbedtls_rsa_rsassa_pss_sign(ds4_key, rng, nullptr, MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA256,
                                  sizeof(hashed_nonce), hashed_nonce, nonce_signature);

  if (rc < 0) {
    LOG_ERR("sign_nonce: failed to sign: mbed error = %d", rc);
    return;
  }

  dump_allocator_hwm();

  LOG_INF("sign_nonce: finished signing");

#if defined(CONFIG_PASSINGLINK_CHECK_MAIN_STACK_HWM)
  const char* main_stack = reinterpret_cast<const char*>(&z_main_stack);
  const char* main_stack_hwm = main_stack;
  while (*main_stack_hwm == 0xAA) {
    ++main_stack_hwm;
  }
  LOG_INF("main stack hwm: %zd bytes", CONFIG_MAIN_STACK_SIZE - (main_stack_hwm - main_stack));
#endif

  current_state = new_state;
  new_state.type = AuthStateType::SendingSignature;
  new_state.next_part = 0;

  if (!auth_state.cas(current_state, new_state)) {
    LOG_ERR("sign_nonce: failed to exchange final auth state");
  }
}

bool set_nonce(u8_t nonce_id, u8_t nonce_part, span<u8_t> data) {
  if (!ds4_key) {
    LOG_ERR("set_nonce: no signing key available");
    return false;
  }

  AuthState current_state = get_auth_state();
  if (current_state.type != AuthStateType::ReceivingNonce) {
    LOG_ERR("set_nonce: received nonce in incorrect state: %u",
            static_cast<u8_t>(current_state.type));
    return false;
  }

  if (nonce_part > 4) {
    LOG_ERR("set_nonce: invalid nonce packet: part = %u", nonce_part);
    return false;
  }

  if (nonce_part != 0 && nonce_id != current_state.nonce_id) {
    LOG_ERR("set_nonce: received a nonce part with mismatching id: expected %u, got %u",
            current_state.nonce_id, nonce_id);
    return false;
  }

  LOG_INF("set_nonce: received data for nonce %u, part %u/5", nonce_id, nonce_part + 1);
  u8_t* begin = nonce + nonce_part * 56;
  memcpy(begin, data.data(), data.size());

  AuthState new_state = current_state;
  bool done = false;
  if (nonce_part == 4) {
    done = true;
    new_state.type = AuthStateType::WaitingToSign;
  } else {
    if (nonce_part == 0) {
      new_state.nonce_id = nonce_id;
    } else {
      ++new_state.next_part;
    }
  }

  bool cas_result = auth_state.cas(current_state, new_state);
  if (!cas_result) {
    LOG_ERR("auth state changed while receiving nonce?");
    return false;
  }

  if (done) {
    LOG_INF("starting signing task");
    k_work_submit_to_queue(&k_main_work_q, &k_work_sign);
  }

  return true;
}

static const u8_t padding[24] = {0};

#define SIGNATURE_BLOCKS()                         \
  SIGNATURE_BLOCK(nonce_sig, 256, nonce_signature) \
  SIGNATURE_BLOCK(serial, 16, ds4_serial)          \
  SIGNATURE_BLOCK(n, 256, ds4_key_n)               \
  SIGNATURE_BLOCK(e, 256, ds4_key_e)               \
  SIGNATURE_BLOCK(key_sig, 256, ds4_signature)     \
  SIGNATURE_BLOCK(padding, 24, padding)

#define SIGNATURE_BLOCK(name, block_size, source)            \
  static size_t copy_##name(span<u8_t> buf, size_t offset) { \
    size_t bytes = block_size - offset;                      \
    if (bytes > buf.size()) {                                \
      bytes = buf.size();                                    \
    }                                                        \
    memcpy(buf.data(), (source) + offset, bytes);            \
    return bytes;                                            \
  }

SIGNATURE_BLOCKS()
#undef SIGNATURE_BLOCK

bool get_next_signature_chunk(span<u8_t> buf) {
  if (buf.size() != 56) {
    LOG_ERR("get_next_signature_chunk: invalid buffer size: %zu", buf.size());
    return false;
  }

  AuthState current_state = auth_state.load();
  LOG_DBG("get_next_signature_chunk: nonce %zu, part %zu started", current_state.nonce_id,
          current_state.next_part);
  size_t current_offset = current_state.next_part * 56;

  if (current_state.type != AuthStateType::SendingSignature) {
    LOG_ERR("get_next_signature_chunk: called when signature not ready (current state = %d)",
            static_cast<int>(current_state.type));
    return false;
  }

  while (buf.size() > 0) {
    size_t prev_block_end = 0;
    // clang-format off
#define SIGNATURE_BLOCK(name, size, source)                  \
    if (current_offset < prev_block_end + size) {            \
      /* We have bytes left. */                              \
      size_t block_offset = current_offset - prev_block_end; \
      size_t bytes_copied = copy_##name(buf, block_offset);  \
      buf.remove_prefix(bytes_copied);                       \
      current_offset += bytes_copied;                        \
      continue;                                              \
    }                                                        \
    prev_block_end += size;
    // clang-format on

    SIGNATURE_BLOCKS()

#undef SIGNATURE_BLOCK
  }

  AuthState new_state = current_state;
  if (++new_state.next_part == 19) {
    new_state.type = AuthStateType::ReceivingNonce;
    new_state.next_part = 0;
  }

  if (!auth_state.cas(current_state, new_state)) {
    LOG_ERR("get_next_signature_chunk: failed to update current state");
    return false;
  }

  LOG_DBG("get_next_signature_chunk: finished writing chunk");
  return true;
}

}  // namespace ps4

#endif
