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
bool input_get_state(InputState* out) {
  memset(out, 0, sizeof(*out));
  out->stick_state = static_cast<u32_t>(StickState::Neutral);
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
  out->button_select = in->button_select;
  out->button_start = in->button_start;
  out->button_home = in->button_home;
  out->button_touchpad = in->button_touchpad;

  // Assign stick.
  int stick_vertical = 0;
  int stick_horizontal = 0;

  // TODO: Make SOCD cleaning customizable.
  if (in->stick_up) {
    ++stick_vertical;
  }
  if (in->stick_down) {
    --stick_vertical;
  }
  if (in->stick_right) {
    ++stick_horizontal;
  }
  if (in->stick_left) {
    --stick_horizontal;
  }

  StickState stick_state;
  if (stick_vertical == 1 && stick_horizontal == 0) {
    stick_state = StickState::North;
  } else if (stick_vertical == 1 && stick_horizontal == 1) {
    stick_state = StickState::NorthEast;
  } else if (stick_vertical == 0 && stick_horizontal == 1) {
    stick_state = StickState::East;
  } else if (stick_vertical == -1 && stick_horizontal == 1) {
    stick_state = StickState::SouthEast;
  } else if (stick_vertical == -1 && stick_horizontal == 0) {
    stick_state = StickState::South;
  } else if (stick_vertical == -1 && stick_horizontal == -1) {
    stick_state = StickState::SouthWest;
  } else if (stick_vertical == 0 && stick_horizontal == -1) {
    stick_state = StickState::West;
  } else if (stick_vertical == 1 && stick_horizontal == -1) {
    stick_state = StickState::NorthWest;
  } else {
    stick_state = StickState::Neutral;
  }

  out->left_stick_x = 127;
  out->left_stick_y = 127;
  out->right_stick_x = 127;
  out->right_stick_y = 127;
  out->dpad = stick_state;
  return true;
}

bool input_get_state(InputState* out) {
  RawInputState input;
  return input_get_raw_state(&input) && input_parse(out, &input);
}

#endif
