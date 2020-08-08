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
uint32_t led_set(Led led, bool value, optional<uint32_t> expected_counter = {});

uint32_t led_on(Led led, optional<uint32_t> expected_counter = {});
uint32_t led_off(Led led, optional<uint32_t> expected_counter = {});

void led_flash(Led led, uint32_t duration_ms, uint32_t interval_ms);
