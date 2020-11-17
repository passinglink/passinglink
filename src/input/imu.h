#pragma once

#include <types.h>

struct IMUVec3_32 {
  union {
    struct {
      int32_t x;
      int32_t y;
      int32_t z;
    };
    struct {
      int32_t pitch;
      int32_t yaw;
      int32_t roll;
    };
  };
};

struct InternalIMUData {
  uint32_t gyro_timestamp;
  IMUVec3_32 gyro;
  uint32_t accel_timestamp;
  IMUVec3_32 accel;
};

extern InternalIMUData imu_data;

void input_imu_init();

// Update the IMU state.
void input_imu_poll();

// Automatically update the local data when there's a sensor update IRQ.
void input_imu_on_irq();
