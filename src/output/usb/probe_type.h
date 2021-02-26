#pragma once

#include <stdint.h>

enum class ProbeType : uint64_t {
  NX = 0xAAAAAAAAAAAAAAAA,
  PS3 = 0xA0A0A0A0A0A0A0A0,
  PS4 = 0x5555555555555555,
};
