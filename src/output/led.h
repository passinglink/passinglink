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

// Set an LED value.
// A counter value is returned, which allows for the ability to undo an operation without affecting
// things if another operation came in after the initial operation.
u32_t led_set(Led led, bool value, optional<u32_t> expected_counter = {});

u32_t led_on(Led led, optional<u32_t> expected_counter = {});
u32_t led_off(Led led, optional<u32_t> expected_counter = {});

void led_flash(Led led, u32_t duration_ms, u32_t interval_ms);
