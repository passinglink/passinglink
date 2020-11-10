#include "input/touchpad.h"

#include <zephyr.h>

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(touchpad);

#if !defined(CONFIG_I2C)
#error I2C touchpad enabled without I2C?
#endif

#include "arch.h"
#include "profiling.h"
#include "types.h"

#define TP_I2C_ADDRESS 0x38

static const struct device* tp_i2c_device;
static const struct device* tp_rst_device;

struct TouchpadXYFormat {
  uint8_t xh : 4;
  uint8_t yh : 4;
  uint8_t xl : 4;
  uint8_t xm : 4;
  uint8_t yl : 4;
  uint8_t ym : 4;
};

#define TP_OUTPUT_REGISTER 0x00
struct TouchpadOutput {
  uint8_t unknown[3];
  uint8_t touchpoints;
  TouchpadXYFormat p1;
  TouchpadXYFormat p2;
};

static void tp_reset() {
  LOG_INF("resetting touchpad");
  gpio_pin_set(tp_rst_device, DT_GPIO_KEYS_TP_RST_GPIOS_PIN, 0);
  k_sleep(K_MSEC(10));
  gpio_pin_set(tp_rst_device, DT_GPIO_KEYS_TP_RST_GPIOS_PIN, 1);
  k_sleep(K_MSEC(10));
}

static bool succeeded_once;
static bool disabled;
static uint8_t attempts;

static uint8_t counter;
void input_touchpad_poll() {
  if (disabled) {
    return;
  }

  // We get polled at 1000Hz by USB.
  // Divide this by 8, since this isn't critical.
  if (++counter == 8) {
    counter = 0;
  } else {
    return;
  };

  PROFILE("input_touchpad_poll", 128);
  TouchpadData* output = &touchpad_data;
  TouchpadOutput input;

  uint8_t reg = TP_OUTPUT_REGISTER;
  input.touchpoints = 0;

  int rc = i2c_write_read(tp_i2c_device, TP_I2C_ADDRESS, &reg, sizeof(reg), &input, sizeof(input));

  if (rc != 0) {
    if (!succeeded_once) {
      if (attempts++ > 128) {
        LOG_ERR("touchpad: not found, disabling");
        disabled = true;
        return;
      }
    } else {
      LOG_WRN("failed to read TP_OUTPUT_REGISTER: rc = %d", rc);
    }
  } else {
    if (input.touchpoints == 0) {
      LOG_DBG("no touchpoints");
      output->p1.unpressed = 1;
    }

    if (input.touchpoints >= 1) {
      if (output->p1.unpressed) {
        ++output->p1.counter;
        output->p1.unpressed = 0;
      }

      uint16_t p1_x = static_cast<uint16_t>(input.p1.xh) << 8 | input.p1.xm << 4 | input.p1.xl;
      uint16_t p1_y = static_cast<uint16_t>(input.p1.yh) << 8 | input.p1.ym << 4 | input.p1.yl;
      output->p1.set_x(p1_x);
      output->p1.set_y(p1_y);
    } else {
      output->p1.unpressed = 1;
    }

    // TODO: Implement multitouch.
  }
}

void input_touchpad_init() {
  tp_i2c_device = device_get_binding(DT_ALIAS_TP_I2C_LABEL);
  tp_rst_device = device_get_binding(DT_GPIO_KEYS_TP_RST_GPIOS_CONTROLLER);
  gpio_pin_configure(tp_rst_device, DT_GPIO_KEYS_TP_RST_GPIOS_PIN,
                     DT_GPIO_KEYS_TP_RST_GPIOS_FLAGS | GPIO_OUTPUT);
  tp_reset();

  touchpad_data.p1.unpressed = 1;
  touchpad_data.p2.unpressed = 1;
}
