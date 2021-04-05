#include "output/usb/ps4/auth.h"

#include <assert.h>
#include <zephyr.h>

#include <kernel.h>
#include <logging/log.h>

#if defined(CONFIG_PASSINGLINK_OUTPUT_USB_PS4_AUTH)
#include <mbedtls/error.h>
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>

#include "panic.h"
#include "provisioning.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(PS4Auth);

extern "C" void dump_allocator_hwm();

#if defined(CONFIG_PASSINGLINK_CHECK_MAIN_STACK_HWM)
extern "C" K_THREAD_STACK_DEFINE(z_main_stack, CONFIG_MAIN_STACK_SIZE);
#endif

static void sign_nonce(struct k_work*);

static atomic_u32<AuthState> auth_state;
static uint8_t nonce[256];
static uint8_t nonce_signature[256];

K_WORK_DEFINE(k_work_sign, sign_nonce);

AuthState get_auth_state() {
  return auth_state.load();
}

static void sign_nonce(struct k_work*) {
  LOG_INF("sign_nonce: started");
  const ProvisioningData* pd = provisioning_data_get();

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

  uint8_t hashed_nonce[32];
  if (mbedtls_sha256_ret(nonce, sizeof(nonce), hashed_nonce, 0) != 0) {
    LOG_ERR("sign_nonce: failed to hash nonce");
    return;
  }

  int rc = mbedtls_rsa_rsassa_pss_sign(pd->ps4_key->rsa_context, rng, nullptr, MBEDTLS_RSA_PRIVATE,
                                       MBEDTLS_MD_SHA256, sizeof(hashed_nonce), hashed_nonce,
                                       nonce_signature);

  if (rc < 0) {
    LOG_ERR("sign_nonce: failed to sign: mbed error = %d", rc);
    return;
  }

  dump_allocator_hwm();

  LOG_INF("sign_nonce: finished signing");

#if defined(CONFIG_PASSINGLINK_CHECK_MAIN_STACK_HWM)
  const uint8_t* main_stack = reinterpret_cast<const uint8_t*>(&z_main_stack);
  const uint8_t* main_stack_hwm = main_stack;
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

bool set_nonce(uint8_t nonce_id, uint8_t nonce_part, span<uint8_t> data) {
  const ProvisioningData* pd = provisioning_data_get();
  if (!pd) {
    LOG_ERR("set_nonce: provisioning partition not available");
    return false;
  }

  if (!pd->ps4_key) {
    LOG_ERR("set_nonce: no signing key available");
    return false;
  }

  AuthState current_state = get_auth_state();
  if (current_state.type != AuthStateType::ReceivingNonce) {
    LOG_ERR("set_nonce: received nonce in incorrect state: %u",
            static_cast<uint8_t>(current_state.type));
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
  uint8_t* begin = nonce + nonce_part * 56;
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

struct SignaturePart {
  size_t length;
  size_t (*function)(span<uint8_t> buf, size_t offset, const void* arg, size_t expected_size);
  const void* arg;
};

static size_t copy_generic(span<uint8_t> buf, size_t offset, const void* arg,
                           size_t expected_size) {
  size_t bytes = expected_size - offset;
  if (bytes > buf.size()) {
    bytes = buf.size();
  }
  memcpy(buf.data(), static_cast<const char*>(arg) + offset, bytes);
  return bytes;
}

static size_t copy_mpi(span<uint8_t> buf, size_t offset, const void* arg, size_t expected_size) {
  assert(expected_size == 256);
  const mbedtls_mpi* mpi = static_cast<const mbedtls_mpi*>(arg);
  unsigned char data[256];
  if (mbedtls_mpi_write_binary(mpi, data, sizeof(data)) != 0) {
    PANIC("failed to write mbedtls_mpi");
  }
  return copy_generic(buf, offset, data, expected_size);
}

static void copy_signature(span<uint8_t> buf, size_t offset) {
  static const uint8_t padding[24] = { 0 };
  const ProvisioningData* pd = provisioning_data_get();
  SignaturePart parts[] = {
    { 256, copy_generic, nonce_signature },
    { 16, copy_generic, pd->ps4_key->serial },
    { 256, copy_mpi, &pd->ps4_key->rsa_context->N },
    { 256, copy_mpi, &pd->ps4_key->rsa_context->E },
    { 256, copy_generic, pd->ps4_key->signature },
    { 24, copy_generic, padding },
  };

  LOG_DBG("copy_signature: buffer = %zu, offset = %zu", buf.size(), offset);

  for (auto& part : parts) {
    size_t number = &part - parts;
    LOG_DBG("copy_signature: part %zu (%zu), offset is currently %zu", number, part.length, offset);
    if (offset >= part.length) {
      LOG_DBG("copy_signature: part %zu: skipping", number);
      offset -= part.length;
      continue;
    }

    size_t bytes_copied = part.function(buf, offset, part.arg, part.length);
    LOG_DBG("copy_signature: part %zu: copied %zu bytes", number, bytes_copied);
    buf.remove_prefix(bytes_copied);
    offset = 0;

    if (buf.empty()) {
      break;
    }
  }

  if (!buf.empty()) {
    PANIC("ran out of signature parts?");
  }
}

bool get_next_signature_chunk(span<uint8_t> buf) {
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

  copy_signature(buf, current_offset);

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

#endif  // CONFIG_PASSINGLINK_OUTPUT_USB_PS4_AUTH
