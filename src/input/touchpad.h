#pragma once

#include <types.h>

struct TouchpadXY {
  uint8_t counter : 7;
  uint8_t unpressed : 1;

  // 12 bit X, followed by 12 bit Y
  uint8_t data[3];

  void set_x(uint16_t x) {
    data[0] = x & 0xff;
    data[1] = (data[1] & 0xf0) | ((x >> 8) & 0xf);
  }

  void set_y(uint16_t y) {
    data[1] = (data[1] & 0x0f) | ((y & 0xf) << 4);
    data[2] = y >> 4;
    return;
  }
};

struct TouchpadData {
  TouchpadXY p1;
  TouchpadXY p2;
};

extern TouchpadData touchpad_data;

void input_touchpad_init();

// Update the touchpad state.
void input_touchpad_poll();
