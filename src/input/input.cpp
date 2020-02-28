#include "input/input.h"

#include <zephyr.h>

#include <device.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(input);

#include "arch.h"
#include "profiling.h"

#if defined(CONFIG_PASSINGLINK_INPUT_NONE)

void input_init() {}
bool input_get_raw_state(RawInputState* out) {
  memset(out, 0, sizeof(*out));
  return true;
}

#else

#define GPIO_PORT_COUNT 4

static struct device* gpio_devices[GPIO_PORT_COUNT];
static u8_t gpio_device_count;

static u8_t gpio_indices[PL_GPIO_COUNT];

static u8_t gpio_device_add(struct device* device) {
  u8_t i;
  for (i = 0; i < GPIO_PORT_COUNT; ++i) {
    if (device == gpio_devices[i]) {
      // Skipping already-cached device.
      return i;
    }
  }
  if (gpio_device_count == GPIO_PORT_COUNT) {
    LOG_ERR("ran out of cached GPIO device slots");
    k_panic();
  }

  i = gpio_device_count++;
  gpio_devices[i] = device;
  return i;
}

void input_init() {
#define PL_GPIO(index, name, NAME)                                      \
  {                                                                     \
    const char* device_name = DT_GPIO_KEYS_##NAME##_GPIOS_CONTROLLER;   \
    struct device* device = device_get_binding(device_name);            \
    gpio_pin_configure(device, DT_GPIO_KEYS_##NAME##_GPIOS_PIN,         \
                       DT_GPIO_KEYS_##NAME##_GPIOS_FLAGS | GPIO_INPUT); \
    u8_t device_offset = gpio_device_add(device);                       \
    gpio_indices[index] = device_offset;                                \
  }
  PL_GPIOS()
#undef PL_GPIO
}

bool input_get_raw_state(RawInputState* out) {
  PROFILE("input_get_raw_state", 128);

  gpio_port_value_t port_values[GPIO_PORT_COUNT];
  for (size_t i = 0; i < gpio_device_count; ++i) {
    gpio_port_get(gpio_devices[i], &port_values[i]);
  }

#define PL_GPIO(index, gpio_name, GPIO_NAME)                                               \
  {                                                                                        \
    const char* device_name = DT_GPIO_KEYS_##GPIO_NAME##_GPIOS_CONTROLLER;                 \
    u8_t device_index = gpio_indices[index];                                               \
    bool value = port_values[device_index] & (1U << DT_GPIO_KEYS_##GPIO_NAME##_GPIOS_PIN); \
    out->gpio_name = value;                                                                \
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

  k_panic();
}

// Scale {-1, 0, 1} to {-128, 0, 127}.
static u8_t stick_scale(int sign) {
  if (sign < 0) {
    return 0x00;
  } else if (sign == 0) {
    return 0x80;
  } else {
    return 0xFF;
  }
}

bool input_parse(InputState* out, const RawInputState* in) {
  PROFILE("input_parse", 128);

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

  out->button_select = in->button_select;
  out->button_start = in->button_start;
  out->button_home = in->button_home;
#if defined(PL_GPIO_MODE_LOCK_AVAILABLE)
  if (in->mode_lock) {
    out->button_select = 0;
    out->button_start = 0;
    out->button_touchpad = 0;
  }
#endif

  // Assign stick.
  int stick_vertical = 0;
  int stick_horizontal = 0;

  // Note: positive Y means down.
  // TODO: Make SOCD cleaning customizable.
  if (in->stick_up) {
    --stick_vertical;
  }
  if (in->stick_down) {
    ++stick_vertical;
  }
  if (in->stick_right) {
    ++stick_horizontal;
  }
  if (in->stick_left) {
    --stick_horizontal;
  }

  enum class OutputMode {
    MODE_DPAD,
    MODE_LS,
    MODE_RS,
  };

  OutputMode output_mode = OutputMode::MODE_DPAD;
#define PL_GPIO(index, mode, MODE)  \
  else if (in->mode) {              \
    output_mode = OutputMode::MODE; \
  }
  if (false) {
  }
  PL_GPIO_OUTPUT_MODES();
#undef PL_GPIO

  out->dpad = StickState::Neutral;
  out->left_stick_x = 128;
  out->left_stick_y = 128;
  out->right_stick_x = 128;
  out->right_stick_y = 128;

  switch (output_mode) {
    case OutputMode::MODE_DPAD:
      out->dpad = stick_state_from_x_y(stick_horizontal, stick_vertical);
      break;

    case OutputMode::MODE_LS:
      out->left_stick_x = stick_scale(stick_horizontal);
      out->left_stick_y = stick_scale(stick_vertical);
      break;

    case OutputMode::MODE_RS:
      out->right_stick_x = stick_scale(stick_horizontal);
      out->right_stick_y = stick_scale(stick_vertical);
      break;
  }

  return true;
}

bool input_get_state(InputState* out) {
  RawInputState input;
  return input_get_raw_state(&input) && input_parse(out, &input);
}
