#include "output/led.h"

#include <kernel.h>
#include <logging/log.h>
#include <types.h>

#include <drivers/gpio.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(led);

struct LedState {
  k_delayed_work work;
  device* led_device;
  u32_t led_pin;
  bool on;
  bool flashing;
  u32_t duration_ticks;
  u32_t interval_ticks;

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
  k_delayed_work_submit_ticks(&state.work, state.interval_ticks);
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

static void led_init(LedState& state, const char* device_name, u32_t gpio_pin, u32_t flags) {
  k_delayed_work_init(&state.work, led_update);
  if (device_name) {
    state.led_device = device_get_binding(device_name);
    state.led_pin = gpio_pin;
    gpio_pin_configure(state.led_device, gpio_pin, GPIO_OUTPUT_INACTIVE | flags);
  }
}

void led_init() {
#define LED_INIT(idx, led_name)                                         \
  led_init(led_states[idx], DT_GPIO_LEDS_##led_name##_GPIOS_CONTROLLER, \
           DT_GPIO_LEDS_##led_name##_GPIOS_PIN, DT_GPIO_LEDS_##led_name##_GPIOS_FLAGS)
#if defined(DT_GPIO_LEDS_LED_0_GPIOS)
  LED_INIT(0, LED_0);
#endif
#if defined(DT_GPIO_LEDS_LED_1_GPIOS)
  LED_INIT(1, LED_1);
#endif
#if defined(DT_GPIO_LEDS_LED_2_GPIOS)
  LED_INIT(2, LED_2);
#endif
#if defined(DT_GPIO_LEDS_LED_3_GPIOS)
  LED_INIT(3, LED_3);
#endif
#if defined(DT_GPIO_LEDS_LED_4_GPIOS)
  LED_INIT(4, LED_4);
#endif
}

void led_on(Led led) {
  LedState& state = led_states[static_cast<size_t>(led)];
  state.on = true;
  if (!state.flashing) {
    led_update(reinterpret_cast<k_work*>(&state.work));
  }
}

void led_off(Led led) {
  LedState& state = led_states[static_cast<size_t>(led)];
  state.on = false;
  if (!state.flashing) {
    led_update(reinterpret_cast<k_work*>(&state.work));
  }
}

void led_flash(Led led, u32_t duration_ms, u32_t interval_ms) {
  LedState& state = led_states[static_cast<size_t>(led)];
  state.flashing = true;
  state.duration_ticks = k_ms_to_ticks_ceil32(duration_ms);
  state.interval_ticks = k_ms_to_ticks_ceil32(interval_ms);
  led_schedule_update(state);
}
