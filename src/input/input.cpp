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
#define PL_BUTTON_GPIO(name, NAME) out->button_##name = 0;
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO
  out->stick_state = static_cast<u32_t>(StickState::Neutral);
  return true;
}

#else

#if defined(STM32F1)
#define GPIO_PORT_COUNT 4
#endif

#if defined(GPIO_PORT_COUNT)
static struct {
  size_t count;
  struct device* device;
} gpio_devices[GPIO_PORT_COUNT];

static size_t gpio_device_count;

static void gpio_device_add(struct device* device) {
  size_t i;
  for (i = 0; i < GPIO_PORT_COUNT; ++i) {
    if (device == gpio_devices[i].device) {
      // Skipping already-cached device.
      ++gpio_devices[i].count;
      return;
    }
  }
  if (gpio_device_count == GPIO_PORT_COUNT) {
    LOG_ERR("ran out of cached GPIO device slots");
    k_panic();
  }
  gpio_devices[gpio_device_count].count = 1;
  gpio_devices[gpio_device_count].device = device;
  ++gpio_device_count;
}
#else
void gpio_device_add(struct device*) {}
#endif

void input_init() {
#define PL_GPIO_INIT(type, NAME)                                                 \
  {                                                                              \
    const char* device_name = DT_GPIO_KEYS_##type##_##NAME##_GPIOS_CONTROLLER;   \
    struct device* device = device_get_binding(device_name);                     \
    gpio_pin_configure(device, DT_GPIO_KEYS_##type##_##NAME##_GPIOS_PIN,         \
                       DT_GPIO_KEYS_##type##_##NAME##_GPIOS_FLAGS | GPIO_INPUT); \
    gpio_device_add(device);                                                     \
  }
#define PL_BUTTON_GPIO(name, NAME) PL_GPIO_INIT(BUTTON, NAME)
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

#define PL_STICK_GPIO(name, NAME) PL_GPIO_INIT(STICK, NAME)
  PL_STICK_GPIOS()
#undef PL_STICK_GPIO

#if defined(GPIO_PORT_COUNT)
  // Sort gpio_device to speed up device finding on average.
  insertion_sort(gpio_devices, gpio_devices + gpio_device_count,
                 [](auto x, auto y) { return x.count < y.count; });
  for (size_t i = 0; i < gpio_device_count; ++i) {
    LOG_DBG("%s: count = %zu", gpio_devices[i].device->config->name, gpio_devices[i].count);
  }
#endif
}

#if defined(GPIO_PORT_COUNT)

// Optimized GPIO reading.
bool input_get_raw_state(RawInputState* out) {
  PROFILE("input_get_raw_state", 128);

  gpio_port_value_t port_values[GPIO_PORT_COUNT];
  for (size_t i = 0; i < gpio_device_count; ++i) {
    gpio_port_get(gpio_devices[i].device, &port_values[i]);
  }

  // TODO: Speed up mapping of pin to port.
  //       What we have here is a lot faster than device_get_binding, but strcmp is still 90% of
  //       our runtime cost.
#define PL_BUTTON_GPIO(gpio_name, GPIO_NAME)                                                     \
  {                                                                                              \
    int port_offset = -1;                                                                        \
    const char* device_name = DT_GPIO_KEYS_BUTTON_##GPIO_NAME##_GPIOS_CONTROLLER;                \
    for (size_t i = 0; i < gpio_device_count; ++i) {                                             \
      if (strcmp(gpio_devices[i].device->config->name, device_name) == 0) {                      \
        port_offset = i;                                                                         \
        break;                                                                                   \
      }                                                                                          \
    }                                                                                            \
    if (port_offset == -1) {                                                                     \
      LOG_ERR("failed to find cached GPIO device %s", device_name);                              \
      k_panic();                                                                                 \
    }                                                                                            \
    bool value = port_values[port_offset] & (1U << DT_GPIO_KEYS_BUTTON_##GPIO_NAME##_GPIOS_PIN); \
    out->button_##gpio_name = value;                                                             \
  }
  PL_BUTTON_GPIOS()
#undef PL_BUTTON_GPIO

#define PL_STICK_GPIO(gpio_name, GPIO_NAME)                                                     \
  {                                                                                             \
    int port_offset = -1;                                                                       \
    const char* device_name = DT_GPIO_KEYS_STICK_##GPIO_NAME##_GPIOS_CONTROLLER;                \
    for (size_t i = 0; i < gpio_device_count; ++i) {                                            \
      if (strcmp(gpio_devices[i].device->config->name, device_name) == 0) {                     \
        port_offset = i;                                                                        \
        break;                                                                                  \
      }                                                                                         \
    }                                                                                           \
    if (port_offset == -1) {                                                                    \
      LOG_ERR("failed to find cached GPIO device %s", device_name);                             \
      k_panic();                                                                                \
    }                                                                                           \
    bool value = port_values[port_offset] & (1U << DT_GPIO_KEYS_STICK_##GPIO_NAME##_GPIOS_PIN); \
    out->stick_##gpio_name = value;                                                             \
  }
  PL_STICK_GPIOS()
#undef PL_STICK_GPIO

  return true;
}

#else  // defined(GPIO_PORT_COUNT)

// Generic, but slow GPIO handling.
bool input_get_raw_state(RawInputState* out) {
  PROFILE("input_get_raw_state", 128);
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

#endif  // defined(GPIO_PORT_COUNT)

bool input_parse(InputState* out, const RawInputState* in) {
  PROFILE("input_parse", 128);

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
