#include "input/imu.h"

#include <zephyr.h>

#include <device.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(imu);

void input_imu_poll() {}
void input_imu_on_irq() {}

void input_imu_init() {
  LOG_INF("IMU disabled");
  imu_data.gyro = {0, 0, 0};
  imu_data.accel = {0, 0, 0};
}
