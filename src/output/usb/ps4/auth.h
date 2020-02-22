#pragma once

#include "types.h"

namespace ps4 {

enum class AuthStateType : u8_t {
  // Waiting for a complete nonce.
  // Transitions to ReadyToSign when the full nonce has been read.
  ReceivingNonce,

  // Waiting for the signing thread to start.
  // Transitions to Signing when signing begins.
  WaitingToSign,

  // Waiting for the signing thread to complete.
  // Transitions to SendingSignature when done, or Resetting upon error.
  Signing,

  // Waiting for the host to read the signature.
  // Transitions to ReceivingNonce when done, or upon error.
  SendingSignature,
};

struct alignas(1) AuthState {
  AuthStateType type;
  u8_t nonce_id;
  u8_t next_part;
  u8_t padding;
};

static_assert(sizeof(AuthState) == 4);

AuthState get_auth_state();
bool set_nonce(u8_t nonce_id, u8_t nonce_part, span<u8_t> data);
bool get_next_signature_chunk(span<u8_t> buf);

}  // namespace ps4
