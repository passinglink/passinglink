#include "input/input.h"

#include <zephyr.h>

#include <device.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_WRN
LOG_MODULE_REGISTER(input);

#if defined(CONFIG_PASSINGLINK_INPUT_NONE)

void input_init() {}
bool input_get_state(InputState* out) {
#define PL_BUTTON_GPIO(name, NAME) out->button_##name = 0;
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO
  out->stick_state = static_cast<u32_t>(StickState::Neutral);
  return true;
}

#else

void input_init() {
#define PL_GPIO_INIT(type, NAME)                                                              \
  {                                                                                           \
    struct device* dev = device_get_binding(DT_GPIO_KEYS_##type##_##NAME##_GPIOS_CONTROLLER); \
    gpio_pin_configure(dev, DT_GPIO_KEYS_##type##_##NAME##_GPIOS_PIN,                         \
                       DT_GPIO_KEYS_##type##_##NAME##_GPIOS_FLAGS | GPIO_INPUT);              \
  }
#define PL_BUTTON_GPIO(name, NAME) PL_GPIO_INIT(BUTTON, NAME)
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

#define PL_STICK_GPIO(name, NAME) PL_GPIO_INIT(STICK, NAME)
  PL_STICK_GPIOS()
#undef PL_STICK_GPIO
}

bool input_get_raw_state(RawInputState* out) {
  // Assign buttons.
#define PL_BUTTON_GPIO(name, NAME)                                                          \
  {                                                                                         \
    struct device* dev = device_get_binding(DT_GPIO_KEYS_BUTTON_##NAME##_GPIOS_CONTROLLER); \
    int rc = gpio_pin_get(dev, DT_GPIO_KEYS_BUTTON_##NAME##_GPIOS_PIN);                     \
    if (rc < 0) {                                                                           \
      LOG_ERR("failed to read GPIO for button " #name ": rc = %d", rc);                     \
      return false;                                                                         \
    }                                                                                       \
    out->button_##name = rc;                                                                \
  }
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

  // Assign stick.
#define PL_STICK_GPIO(name, NAME)                                                          \
  bool stick_##name = false;                                                               \
  {                                                                                        \
    struct device* dev = device_get_binding(DT_GPIO_KEYS_STICK_##NAME##_GPIOS_CONTROLLER); \
    int rc = gpio_pin_get(dev, DT_GPIO_KEYS_STICK_##NAME##_GPIOS_PIN);                     \
    if (rc < 0) {                                                                          \
      LOG_ERR("failed to read GPIO for stick " #name ": rc = %d", rc);                     \
      return false;                                                                        \
    }                                                                                      \
    out->stick_##name = rc;                                                                \
  }
  PL_STICK_GPIOS()
#undef PL_STICK_GPIO

  return true;
}

bool input_parse(InputState* out, const RawInputState* in) {
  // Assign buttons.
#define PL_BUTTON_GPIO(name, NAME) out->button_##name = in->button_##name;
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

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
