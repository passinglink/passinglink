#include "input/input.h"

#include <zephyr.h>

#include <device.h>
#include <devicetree/gpio.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(input);

#include "arch.h"
#include "display/display.h"
#include "input/queue.h"
#include "input/touchpad.h"
#include "panic.h"
#include "profiling.h"
#include "types.h"

TouchpadData touchpad_data;

static void input_gpio_init();

void input_init() {
  input_gpio_init();
  input_touchpad_init();
}

#if defined(CONFIG_PASSINGLINK_INPUT_NONE)

static void input_gpio_init() {}

bool input_get_raw_state(RawInputState* out) {
#if defined(CONFIG_PASSINGLINK_INPUT_QUEUE)
  if (auto input = input_queue_get_state()) {
    *out = *input;
    return true;
  }
#endif

  memset(out, 0, sizeof(*out));
  return true;
}

#elif defined(CONFIG_PASSINGLINK_INPUT_EXTERNAL)

static RawInputState input_state;

static void input_gpio_init() {}

bool input_get_raw_state(RawInputState* out) {
#if defined(CONFIG_PASSINGLINK_INPUT_QUEUE)
  if (auto input = input_queue_get_state()) {
    *out = *input;
    return true;
  }
#endif

  *out = input_state;
  return true;
}

void input_set_raw_state(RawInputState* in) {
  input_state = *in;
}

#else

#define GPIO_PORT_COUNT 4

static const struct device* gpio_devices[GPIO_PORT_COUNT];
static uint8_t gpio_device_count;

static uint8_t gpio_indices[PL_GPIO_COUNT];

static uint8_t gpio_device_add(const struct device* device) {
  uint8_t i;
  for (i = 0; i < GPIO_PORT_COUNT; ++i) {
    if (device == gpio_devices[i]) {
      // Skipping already-cached device.
      return i;
    }
  }
  if (gpio_device_count == GPIO_PORT_COUNT) {
    PANIC("ran out of cached GPIO device slots");
  }

  i = gpio_device_count++;
  gpio_devices[i] = device;
  return i;
}

static void input_gpio_init() {
#define PL_GPIO(index, name)                                                                    \
  {                                                                                             \
    const struct device* device = device_get_binding(PL_GPIO_LABEL(name));                      \
    if (!device) {                                                                              \
      PANIC("failed to find gpio device %s", PL_GPIO_LABEL(name));                              \
    }                                                                                           \
    if (gpio_pin_configure(device, PL_GPIO_PIN(name), PL_GPIO_FLAGS(name) | GPIO_INPUT) != 0) { \
      PANIC("failed to configure gpio pin (device = %s, pin = %d)", PL_GPIO_LABEL(name),        \
            PL_GPIO_PIN(name));                                                                 \
    };                                                                                          \
    uint8_t device_offset = gpio_device_add(device);                                            \
    gpio_indices[index] = device_offset;                                                        \
  }
  PL_GPIOS()
#undef PL_GPIO
}

bool input_get_raw_state(RawInputState* out) {
  PROFILE("input_get_raw_state", 128);

#if defined(CONFIG_PASSINGLINK_INPUT_QUEUE)
  if (auto input = input_queue_get_state()) {
    *out = *input;
    return true;
  }
#endif

  gpio_port_value_t port_values[GPIO_PORT_COUNT];
  for (size_t i = 0; i < gpio_device_count; ++i) {
    if (gpio_port_get_raw(gpio_devices[i], &port_values[i]) != 0) {
      PANIC("failed to get gpio values");
    }
  }

#define PL_GPIO(index, name)                                            \
  {                                                                     \
    uint8_t device_index = gpio_indices[index];                         \
    bool value = port_values[device_index] & (1U << PL_GPIO_PIN(name)); \
    if constexpr (PL_GPIO_FLAGS(name) & GPIO_ACTIVE_LOW) {              \
      out->name = !value;                                               \
    } else {                                                            \
      out->name = value;                                                \
    }                                                                   \
  }
  PL_GPIOS()
#undef PL_GPIO

  return true;
}
#endif

// Convert ({-1, 0, 1}, {-1, 0, 1}) to a StickState.
static StickState stick_state_from_x_y(int horizontal, int vertical) {
  if (vertical == -1 && horizontal == 0) {
    return StickState::North;
  } else if (vertical == -1 && horizontal == 1) {
    return StickState::NorthEast;
  } else if (vertical == 0 && horizontal == 1) {
    return StickState::East;
  } else if (vertical == 1 && horizontal == 1) {
    return StickState::SouthEast;
  } else if (vertical == 1 && horizontal == 0) {
    return StickState::South;
  } else if (vertical == 1 && horizontal == -1) {
    return StickState::SouthWest;
  } else if (vertical == 0 && horizontal == -1) {
    return StickState::West;
  } else if (vertical == -1 && horizontal == -1) {
    return StickState::NorthWest;
  } else {
    return StickState::Neutral;
  }

  __builtin_unreachable();
}

// Scale {-1, 0, 1} to {-128, 0, 127}.
static uint8_t stick_scale(int sign) {
  if (sign < 0) {
    return 0x00;
  } else if (sign == 0) {
    return 0x80;
  } else {
    return 0xFF;
  }
}

static bool input_locked = false;
static uint64_t input_lock_tick;
bool input_get_locked() {
  return input_locked;
}

static void input_set_locked(bool locked, uint64_t tick) {
#if defined(CONFIG_PASSINGLINK_DISPLAY)
  if (locked != input_locked) {
    display_set_locked(locked);
    input_lock_tick = tick;
  }
#endif

  input_locked = locked;
}

void input_set_locked(bool locked) {
  input_set_locked(locked, k_uptime_ticks());
}

struct ButtonHistory {
  bool state;

  // The cycle on which it entered the state.
  uint64_t tick;
};

struct {
#define PL_GPIO(index, name) ButtonHistory name;
  PL_GPIOS()
#undef PL_GPIO
} button_history;

// Debounce a button input, given its history.
// Updates ButtonHistory and returns the value that should be used.
static bool input_debounce(bool current_state, ButtonHistory* button_history,
                           uint64_t current_tick) {
  if (current_state == button_history->state) {
    return current_state;
  }

  // Only allow transitions every 5 milliseconds.
  // TODO: Make configurable?
  uint64_t duration_ticks = current_tick - button_history->tick;
  constexpr uint64_t transition_time = k_ms_to_cyc_ceil64(5);
  if (duration_ticks < transition_time) {
    return !current_state;
  }

  button_history->state = current_state;
  button_history->tick = current_tick;
  return current_state;
}

struct StickOutput {
  struct Axis {
    // -1 to 1
    int value;

    uint64_t tick;
  };

  Axis x;
  Axis y;
};

enum class SOCDButtonType : int {
  // Down, right
  Positive = 1,

  // Neutral.
  Neutral = 0,

  // Up, left
  Negative = -1,
};

struct SOCDInputs {
  // bool, but bitfields make it so a cast would be needed to silence warnings.
  uint32_t input_value;
  uint64_t input_tick;
  SOCDButtonType button_type;
  bool overrides = false;
};

// TODO: Store these in flash.
static SOCDType input_socd_type_x = SOCDType::Neutral;
static SOCDType input_socd_type_y = SOCDType::Negative;

SOCDType input_get_x_socd_type() {
  return input_socd_type_x;
}

void input_set_x_socd_type(SOCDType type) {
  input_socd_type_x = type;
}

SOCDType input_get_y_socd_type() {
  return input_socd_type_y;
}

void input_set_y_socd_type(SOCDType type) {
  input_socd_type_y = type;
}

StickOutput::Axis input_socd_generic(SOCDType type, span<SOCDInputs> inputs) {
  optional<uint64_t> newest_positive;
  optional<uint64_t> newest_neutral;
  optional<uint64_t> newest_negative;

  for (auto& input : inputs) {
    if (input.input_value) {
      if (input.overrides) {
        return StickOutput::Axis {
          .value = static_cast<int>(input.button_type),
          .tick = input.input_tick
        };
      }
      optional<uint64_t>* target = nullptr;
      switch (input.button_type) {
        case SOCDButtonType::Positive:
          target = &newest_positive;
          break;

        case SOCDButtonType::Neutral:
          target = &newest_neutral;
          break;

        case SOCDButtonType::Negative:
          target = &newest_negative;
          break;
      }

      if (!*target || **target < input.input_tick) {
        target->reset(input.input_tick);
      }
    }
  }

  switch (type) {
    case SOCDType::Neutral:
      return StickOutput::Axis {
        .value = newest_positive.valid() - newest_negative.valid(),
        .tick = max(newest_positive.get_or(0), newest_negative.get_or(0)),
      };

    case SOCDType::Positive:
      if (newest_positive.valid()) {
        return StickOutput::Axis {
          .value = 1,
          .tick = *newest_positive,
        };
      } else if (newest_negative.valid()) {
        return StickOutput::Axis {
          .value = -1,
          .tick = *newest_negative,
        };
      } else {
        return StickOutput::Axis {
          .value = 0,
          .tick = 0,
        };
      }

    case SOCDType::Negative:
      if (newest_negative.valid()) {
        return StickOutput::Axis {
          .value = -1,
          .tick = *newest_negative,
        };
      } else if (newest_positive.valid()) {
        return StickOutput::Axis {
          .value = 1,
          .tick = *newest_positive,
        };
      } else {
        return StickOutput::Axis {
          .value = 0,
          .tick = 0,
        };
      }

    case SOCDType::Last:
      break;
  }

  struct {
    optional<uint64_t> tick;
    int value;
  } results[] = {
    {newest_positive, 1},
    {newest_neutral, 0},
    {newest_negative, -1},
  };

  optional<uint64_t> newest_tick;
  optional<int> newest_value;

  for (auto& x : results) {
    if (!x.tick) continue;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    if (!newest_tick || *newest_tick < *x.tick) {
      newest_tick = *x.tick;
      newest_value.reset(x.value);
    }
  }

  return StickOutput::Axis {
    .value = newest_value.get_or(0),
    .tick = newest_tick.get_or(0),
  };
#pragma GCC diagnostic pop
}

static StickOutput::Axis input_socd_x(const RawInputState* in) {
  SOCDInputs inputs[] = {
    {in->stick_left, button_history.stick_left.tick, SOCDButtonType::Negative},
    {in->stick_right, button_history.stick_right.tick, SOCDButtonType::Positive},
  };

  return input_socd_generic(input_socd_type_x, inputs);
}

static StickOutput::Axis input_socd_y(const RawInputState* in) {
  SOCDInputs inputs[] = {
    {in->stick_up, button_history.stick_up.tick, SOCDButtonType::Negative},
    {in->stick_down, button_history.stick_down.tick, SOCDButtonType::Positive},
  };

  return input_socd_generic(input_socd_type_y, inputs);
}

static StickOutput input_socd(const RawInputState* in) {
  return StickOutput{
    .x = input_socd_x(in),
    .y = input_socd_y(in),
  };
}

static bool input_parse_menu(const RawInputState* in, StickOutput stick, uint64_t current_tick) {
#if defined(PL_GPIO_BUTTON_MENU_AVAILABLE)
  if (!button_history.button_menu.value) {
    if (button_history.button_menu.tick == current_tick) {
      menu_close();
    }
    return false;
  }

  if (input_locked) {
    // TODO: Make timeout configurable.
    // TODO: Display progress bar.
    if (current_tick - button_history.button_menu.tick < k_ms_to_ticks_ceil64(2000)) {
      return false;
    } else if (input_lock_tick < button_history.button_menu.tick) {
      menu_open();
    }
  }

  if (stick.x.value != 0 && stick.x.tick == current_tick) {
    if (stick.x.value == -1) {
      menu_input(MenuInput::Left);
    } else {
      menu_input(MenuInput::Right);
    }
    return true;
  }

  if (stick.y.value != 0 && stick.y.tick == current_tick) {
    if (stick.y.value == -1) {
      menu_input(MenuInput::Up);
    } else {
      menu_input(MenuInput::Down);
    }
    return true;
  }
#endif

  return false;
}

static bool input_parse(InputState* out, RawInputState* in) {
  PROFILE("input_parse", 128);

  // Copy TouchpadData.
  out->touchpad_data = touchpad_data;

  uint64_t current_tick = k_uptime_ticks();
  // Debounce inputs.
#define PL_GPIO(index, name) \
  in->name = input_debounce(in->name, &button_history.name, current_tick);
  PL_GPIOS()
#undef PL_GPIO

#if defined(PL_GPIO_MODE_LOCK_AVAILABLE)
  input_set_locked(in->mode_lock, current_tick);
#endif

  StickOutput stick_output = input_socd(in);
  if (input_parse_menu(in, stick_output, current_tick)) {
    return true;
  }

  // Assign buttons.
  out->button_north = in->button_north;
  out->button_east = in->button_east;
  out->button_south = in->button_south;
  out->button_west = in->button_west;
  out->button_l1 = in->button_l1;
  out->button_l2 = in->button_l2;
  out->button_l3 = in->button_l3;
  out->button_r1 = in->button_r1;
  out->button_r2 = in->button_r2;
  out->button_r3 = in->button_r3;
  out->button_touchpad = in->button_touchpad;

  out->button_select = input_locked ? 0 : in->button_select;
  out->button_start = input_locked ? 0 : in->button_start;
  out->button_home = input_locked ? 0 : in->button_home;

  out->dpad = StickState::Neutral;
  out->left_stick_x = 128;
  out->left_stick_y = 128;
  out->right_stick_x = 128;
  out->right_stick_y = 128;

  enum class OutputMode {
    mode_dpad,
    mode_ls,
    mode_rs,
  };

  OutputMode output_mode = OutputMode::mode_dpad;
#define PL_GPIO(index, mode)        \
  else if (in->mode) {              \
    output_mode = OutputMode::mode; \
  }
  if (false) {
  }
  PL_GPIO_OUTPUT_MODES();
#undef PL_GPIO

  switch (output_mode) {
    case OutputMode::mode_dpad:
      out->dpad = stick_state_from_x_y(stick_output.x.value, stick_output.y.value);
      break;

    case OutputMode::mode_ls:
      out->left_stick_x = stick_scale(stick_output.x.value);
      out->left_stick_y = stick_scale(stick_output.y.value);
      break;

    case OutputMode::mode_rs:
      out->right_stick_x = stick_scale(stick_output.x.value);
      out->right_stick_y = stick_scale(stick_output.y.value);
      break;
  }

  return true;
}

bool input_get_state(InputState* out) {
  RawInputState input;
  return input_get_raw_state(&input) && input_parse(out, &input);
}
