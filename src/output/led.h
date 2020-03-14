#pragma once

#include <types.h>

enum class Led {
  Front,
  P1,
  P2,
  P3,
  P4,
};

void led_init();
void led_on(Led led);
void led_off(Led led);
void led_flash(Led led, u32_t duration_ms, u32_t interval_ms);
