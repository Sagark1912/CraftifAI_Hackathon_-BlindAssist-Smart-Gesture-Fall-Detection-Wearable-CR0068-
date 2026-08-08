#include "mpu6050.h"
#include "app_config.h"
#include "logger.h"
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "mpu6050";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev;
static bool initialized;
static uint8_t active_address;
static float gyro_bias[3];
static float gravity_mag = 1.0f;
static float gravity_lpf[3] = {0.0f, 0.0f, 1.0f};
static float roll_deg;
static float pitch_deg;

static esp_err_t reg_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 50);
}

static esp_err_t reg_write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(dev, buf, sizeof(buf), 50);
}

esp_err_t mpu6050_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = APP_I2C_SDA_GPIO,
        .scl_io_num = APP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (initialized) return ESP_OK;
    ESP_LOGI(TAG, "I2C init: port=%d SDA=%d SCL=%d freq=%d internal_pullups=%s", I2C_NUM_0, APP_I2C_SDA_GPIO, APP_I2C_SCL_GPIO, APP_I2C_FREQUENCY_HZ, bus_cfg.flags.enable_internal_pullup ? "on" : "off");
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus);
    ESP_LOGI(TAG, "i2c_new_master_bus: %s (0x%x), handle=%p", esp_err_to_name(err), err, (void *)bus);
    if (err != ESP_OK) return err;

    uint16_t addresses[2] = {APP_MPU6050_ADDRESS, APP_MPU6050_ADDRESS == 0x68 ? 0x69 : 0x68};
    uint16_t found = 0;
    for (size_t i = 0; i < 2; ++i) {
        esp_err_t probe = i2c_master_probe(bus, addresses[i], 100);
        ESP_LOGI(TAG, "I2C probe 0x%02x: %s (0x%x)", addresses[i], esp_err_to_name(probe), probe);
        if (probe == ESP_OK) { found = addresses[i]; break; }
    }
    if (found == 0) {
        ESP_LOGE(TAG, "No I2C device ACK at 0x68 or 0x69; check SDA/SCL, pull-ups, power, and AD0");
        i2c_del_master_bus(bus);
        bus = NULL;
        return ESP_ERR_NOT_FOUND;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = found,
        .scl_speed_hz = APP_I2C_FREQUENCY_HZ,
    };
    ESP_LOGI(TAG, "I2C device config: address=0x%02x address_length=7-bit speed=%d", dev_cfg.device_address, dev_cfg.scl_speed_hz);
    err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
    ESP_LOGI(TAG, "add device 0x%02x: %s (0x%x), handle=%p", dev_cfg.device_address, esp_err_to_name(err), err, (void *)dev);
    if (err != ESP_OK) {
        i2c_del_master_bus(bus);
        bus = NULL;
        return err;
    }

    uint8_t who = 0;
    err = reg_read(0x75, &who, 1);
    ESP_LOGI(TAG, "WHO_AM_I read at 0x%02x: %s (0x%x), value=0x%02x", dev_cfg.device_address, esp_err_to_name(err), err, who);
    if (err != ESP_OK || (who != 0x68 && who != 0x69)) {
        ESP_LOGE(TAG, "MPU6050 WHO_AM_I failed: 0x%02x", who);
        if (dev) i2c_master_bus_rm_device(dev);
        i2c_del_master_bus(bus);
        dev = NULL;
        bus = NULL;
        return err == ESP_OK ? ESP_ERR_NOT_FOUND : err;
    }
    ESP_ERROR_CHECK(reg_write(0x6B, 0x00));
    ESP_ERROR_CHECK(reg_write(0x1C, 0x10)); /* +/-8 g */
    ESP_ERROR_CHECK(reg_write(0x1B, 0x10)); /* +/-1000 dps */
    ESP_ERROR_CHECK(reg_write(0x1A, 0x03)); /* digital low-pass filter */
    active_address = dev_cfg.device_address;
    initialized = true;
    ESP_LOGI(TAG, "MPU6050 ready at 0x%02x", active_address);
    return ESP_OK;
}

esp_err_t mpu6050_calibrate(uint32_t seconds)
{
    const uint32_t count = seconds * APP_SAMPLE_HZ;
    double sum_g[3] = {0};
    double sum_a[3] = {0};
    uint8_t raw[14];
    for (uint32_t i = 0; i < count; ++i) {
        esp_err_t err = reg_read(0x3B, raw, sizeof(raw));
        if (err != ESP_OK) return err;
        for (int axis = 0; axis < 3; ++axis) {
            int16_t av = (int16_t)((raw[axis * 2] << 8) | raw[axis * 2 + 1]);
            int16_t gv = (int16_t)((raw[8 + axis * 2] << 8) | raw[9 + axis * 2]);
            sum_a[axis] += (float)av / 4096.0f;
            sum_g[axis] += (float)gv / 32.8f;
        }
        vTaskDelay(pdMS_TO_TICKS(1000 / APP_SAMPLE_HZ));
    }
    for (int axis = 0; axis < 3; ++axis) {
        gyro_bias[axis] = (float)(sum_g[axis] / count);
        gravity_lpf[axis] = (float)(sum_a[axis] / count);
    }
    gravity_mag = sqrtf(gravity_lpf[0] * gravity_lpf[0] + gravity_lpf[1] * gravity_lpf[1] + gravity_lpf[2] * gravity_lpf[2]);
    ESP_LOGI(TAG, "calibration complete: gyro bias %.2f %.2f %.2f dps", gyro_bias[0], gyro_bias[1], gyro_bias[2]);
    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_sample_t *sample)
{
    if (!sample) return ESP_ERR_INVALID_ARG;
    uint8_t raw[14];
    esp_err_t err = reg_read(0x3B, raw, sizeof(raw));
    if (err != ESP_OK) return err;
    int16_t v[7];
    for (int i = 0; i < 7; ++i) v[i] = (int16_t)((raw[i * 2] << 8) | raw[i * 2 + 1]);
    sample->ax_g = v[0] / 4096.0f;
    sample->ay_g = v[1] / 4096.0f;
    sample->az_g = v[2] / 4096.0f;
    sample->gx_dps = v[4] / 32.8f - gyro_bias[0];
    sample->gy_dps = v[5] / 32.8f - gyro_bias[1];
    sample->gz_dps = v[6] / 32.8f - gyro_bias[2];
    sample->accel_mag_g = sqrtf(sample->ax_g * sample->ax_g + sample->ay_g * sample->ay_g + sample->az_g * sample->az_g);
    sample->gyro_mag_dps = sqrtf(sample->gx_dps * sample->gx_dps + sample->gy_dps * sample->gy_dps + sample->gz_dps * sample->gz_dps);
    const float alpha = 0.08f;
    gravity_lpf[0] += alpha * (sample->ax_g - gravity_lpf[0]);
    gravity_lpf[1] += alpha * (sample->ay_g - gravity_lpf[1]);
    gravity_lpf[2] += alpha * (sample->az_g - gravity_lpf[2]);
    sample->dynamic_accel_g = fabsf(sample->accel_mag_g - gravity_mag);
    sample->roll_deg = atan2f(gravity_lpf[1], gravity_lpf[2]) * 57.29578f;
    sample->pitch_deg = atan2f(-gravity_lpf[0], sqrtf(gravity_lpf[1] * gravity_lpf[1] + gravity_lpf[2] * gravity_lpf[2])) * 57.29578f;
    roll_deg = sample->roll_deg;
    pitch_deg = sample->pitch_deg;
    return ESP_OK;
}
