#pragma once

#if defined(CONFIG_PASSINGLINK_BT)

// clang-format off
// 1209214d-246d-2815-27c6-f57dad45XXXX
#define PL_BT_UUID_PREFIX \
  0x45, 0xad, 0x7d, 0xf5, 0xc6, 0x27, \
  0x15, 0x28, \
  0x6d, 0x24, \
  0x4d, 0x21, 0x09, 0x12,
// clang-format on

void bluetooth_init();

#endif
