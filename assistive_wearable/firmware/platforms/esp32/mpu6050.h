#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
    float ax_g, ay_g, az_g;
    float gx_dps, gy_dps, gz_dps;
    float accel_mag_g;
    float gyro_mag_dps;
    float dynamic_accel_g;
    float roll_deg, pitch_deg;
} mpu6050_sample_t;

esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read(mpu6050_sample_t *sample);
esp_err_t mpu6050_calibrate(uint32_t seconds);

#endif
