#pragma once

#include <types.h>

struct IMUVec3_16 {
  union {
    struct {
      int16_t x;
      int16_t y;
      int16_t z;
    };
    struct {
      int16_t pitch;
      int16_t yaw;
      int16_t roll;
    };
  };
};

struct InternalIMUData {
  uint32_t gyro_timestamp;
  IMUVec3_16 gyro;
  uint32_t accel_timestamp;
  IMUVec3_16 accel;
};

extern InternalIMUData imu_data;

void input_imu_init();

// Update the IMU state.
void input_imu_poll();

// Automatically update the local data when there's a sensor update IRQ.
void input_imu_on_irq();
