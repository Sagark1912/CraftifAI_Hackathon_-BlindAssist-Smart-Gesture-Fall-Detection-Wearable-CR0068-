#ifndef IMU_IF_H
#define IMU_IF_H
#include <stdint.h>
#include "esp_err.h"
typedef struct { float ax_g, ay_g, az_g, gx_dps, gy_dps, gz_dps, accel_mag_g, gyro_mag_dps, dynamic_accel_g, roll_deg, pitch_deg; } imu_sample_t;
esp_err_t imu_init(void);
esp_err_t imu_calibrate(unsigned seconds);
esp_err_t imu_read(imu_sample_t *sample);
#endif
