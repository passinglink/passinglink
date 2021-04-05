#pragma once

#include "types.h"

enum class AuthStateType : uint8_t {
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
  uint8_t nonce_id;
  uint8_t next_part;
  uint8_t padding;
};

static_assert(sizeof(AuthState) == 4);

AuthState get_auth_state();
bool set_nonce(uint8_t nonce_id, uint8_t nonce_part, span<uint8_t> data);
bool get_next_signature_chunk(span<uint8_t> buf);
