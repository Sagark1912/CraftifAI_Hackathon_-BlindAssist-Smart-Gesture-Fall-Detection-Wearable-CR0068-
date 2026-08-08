#include "imu_if.h"
#include "mpu6050.h"
esp_err_t imu_init(void) { return mpu6050_init(); }
esp_err_t imu_calibrate(unsigned seconds) { return mpu6050_calibrate(seconds); }
esp_err_t imu_read(imu_sample_t *sample) { return mpu6050_read((mpu6050_sample_t *)sample); }
