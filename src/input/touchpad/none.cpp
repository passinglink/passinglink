#include "input/touchpad.h"

#include <zephyr.h>

#include <device.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(touchpad);

#define TP_I2C_ADDRESS 0x38

void input_touchpad_poll() {}

void input_touchpad_init() {
  LOG_INF("touchpad disabled");
  touchpad_data.p1.unpressed = 1;
  touchpad_data.p2.unpressed = 1;
}
