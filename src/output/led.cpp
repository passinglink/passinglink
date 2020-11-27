#include "output/led.h"

#include <kernel.h>
#include <logging/log.h>
#include <types.h>

#include <devicetree/gpio.h>
#include <drivers/gpio.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(led);

struct LedState {
  k_delayed_work work;
  const device* led_device;
  uint32_t led_pin;
  uint32_t counter;

  uint32_t duration_ticks;
  uint32_t interval_ticks;

  bool initialized;
  bool on;
  bool flashing;

  size_t index() const;
};

static LedState led_states[5];

size_t LedState::index() const {
  return this - led_states;
}

static void led_set(LedState& state, bool on) {
  if (state.led_device) {
    gpio_pin_set(state.led_device, state.led_pin, on);
  }
}

static void led_toggle(LedState& state) {
  if (state.led_device) {
    gpio_pin_toggle(state.led_device, state.led_pin);
  }
}

static void led_schedule_update(LedState& state) {
  if (state.initialized) {
    k_delayed_work_submit(&state.work, K_TICKS(state.interval_ticks));
  }
}

void led_update(k_work* work) {
  LedState& state = *reinterpret_cast<LedState*>(work);
  if (state.flashing) {
    if (state.interval_ticks > state.duration_ticks) {
      // We're done.
      led_set(state, state.on);
      state.interval_ticks = 0;
      state.duration_ticks = 0;
      state.flashing = false;
    } else {
      led_toggle(state);
      state.duration_ticks -= state.interval_ticks;
      led_schedule_update(state);
    }
  } else {
    led_set(state, state.on);
  }
}

static void led_init(LedState& state, const char* device_name, uint32_t gpio_pin, uint32_t flags) {
  if (device_name) {
    state.initialized = true;
    state.led_device = device_get_binding(device_name);
    state.led_pin = gpio_pin;
    gpio_pin_configure(state.led_device, gpio_pin, GPIO_OUTPUT_INACTIVE | flags);
    k_delayed_work_init(&state.work, led_update);
  }
}

#define LED_NODE(name) DT_PATH(leds, name)

void led_init() {
#define LED_INIT(idx, led_name)                                       \
  led_init(led_states[idx], DT_GPIO_LABEL(LED_NODE(led_name), gpios), \
           DT_GPIO_PIN(LED_NODE(led_name), gpios), DT_GPIO_FLAGS(LED_NODE(led_name), gpios))
#if DT_NODE_HAS_STATUS(LED_NODE(led_0), okay)
  LED_INIT(0, led_0);
#endif
#if DT_NODE_HAS_STATUS(LED_NODE(led_1), okay)
  LED_INIT(1, led_1);
#endif
#if DT_NODE_HAS_STATUS(LED_NODE(led_2), okay)
  LED_INIT(2, led_2);
#endif
#if DT_NODE_HAS_STATUS(LED_NODE(led_3), okay)
  LED_INIT(3, led_3);
#endif
#if DT_NODE_HAS_STATUS(LED_NODE(led_4), okay)
  LED_INIT(4, led_4);
#endif
}

uint32_t led_set(Led led, bool value, optional<uint32_t> expected_counter) {
  LedState& state = led_states[static_cast<size_t>(led)];
  if (expected_counter && *expected_counter != state.counter) {
    return state.counter;
  }

  state.on = value;
  if (!state.flashing) {
    led_update(reinterpret_cast<k_work*>(&state.work));
  }

  if (expected_counter) {
    return *expected_counter;
  }
  return ++state.counter;
}

uint32_t led_on(Led led, optional<uint32_t> expected_counter) {
  return led_set(led, true, expected_counter);
}

uint32_t led_off(Led led, optional<uint32_t> expected_counter) {
  return led_set(led, false, expected_counter);
}

void led_flash(Led led, uint32_t duration_ms, uint32_t interval_ms) {
  LedState& state = led_states[static_cast<size_t>(led)];
  state.flashing = true;
  state.duration_ticks = k_ms_to_ticks_ceil32(duration_ms);
  state.interval_ticks = k_ms_to_ticks_ceil32(interval_ms);
  led_schedule_update(state);
}
