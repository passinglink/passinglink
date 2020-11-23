#pragma once

#include <stdint.h>

// Provisioning information is stored in a separate partition on flash.
enum class ProvisioningVersion : uint32_t {
  V1 = 0x1209214c,
};

struct PS4Key {
  unsigned char serial[16];
  unsigned char signature[256];
  struct mbedtls_rsa_context* rsa_context;
};

struct __attribute__((packed)) ProvisioningData {
  ProvisioningVersion version;
  char board_name[128];
  PS4Key* ps4_key;
};
