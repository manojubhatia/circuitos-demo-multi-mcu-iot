/**
 * @file AHT20.cpp
 * @brief AHT20 Temperature and Humidity Sensor Implementation (PAL I2C)

 */

#include "AHT20.hpp"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "osal/osal_task.h"

OSAL_LOG_MODULE_REGISTER(AHT20)

namespace embedded {

AHT20::AHT20(uint8_t addr, int sdaPin, int sclPin)
    : Sensor("AHT20"), _address(addr), _sdaPin(sdaPin), _sclPin(sclPin) {}

esp_err_t AHT20::init() {
    if (_initialized)
        return ESP_OK;

    pal_i2c_config_t i2c_cfg = {
        .sda_pin = _sdaPin,
        .scl_pin = _sclPin,
        .speed = PAL_I2C_SPEED_100K,
        .pullup_enable = true,
    };
    int ret = pal_i2c_init(_i2cPort, &i2c_cfg);
    if (ret != 0) {
        OSAL_LOGE("Failed to init I2C port %d", _i2cPort);
        return ESP_FAIL;
    }

    osal_task_delay_ms(40);

    uint8_t initCmd[] = {0xBE, 0x08, 0x00};
    ret = pal_i2c_write(_i2cPort, _address, initCmd, sizeof(initCmd), 100);
    if (ret != 0) {
        OSAL_LOGW("Init command failed (may be OK): %d", ret);
    }

    osal_task_delay_ms(10);

    OSAL_LOGI("Initialized on SDA=%d, SCL=%d", _sdaPin, _sclPin);
    _initialized = true;
    return ESP_OK;
}

esp_err_t AHT20::deinit() {
    pal_i2c_deinit(_i2cPort);
    _initialized = false;
    return ESP_OK;
}

bool AHT20::triggerMeasurement() {
    uint8_t trigCmd[] = {0xAC, 0x33, 0x00};
    int ret = pal_i2c_write(_i2cPort, _address, trigCmd, sizeof(trigCmd), 100);
    return ret == 0;
}

bool AHT20::readRawData(uint8_t *data) {
    int ret = pal_i2c_read(_i2cPort, _address, data, 7, 100);
    return ret == 0;
}

bool AHT20::readData(float *temp, float *humidity) {
    if (!_initialized)
        return false;

    if (!triggerMeasurement())
        return false;

    osal_task_delay_ms(80);

    uint8_t data[7];
    if (!readRawData(data))
        return false;

    if (data[0] & 0x80) {
        osal_task_delay_ms(50);
        if (!readRawData(data))
            return false;
    }

    uint32_t rawHum = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    *humidity = (rawHum / 1048576.0f) * 100.0f;

    uint32_t rawTemp = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *temp = (rawTemp / 1048576.0f) * 200.0f - 50.0f;

    _lastTemp = *temp;
    _lastHumidity = *humidity;

    return true;
}

float AHT20::read() {
    float temp, hum;
    if (readData(&temp, &hum))
        return temp;
    return _lastTemp;
}

float AHT20::readTemperature() {
    float temp, hum;
    if (readData(&temp, &hum))
        return temp;
    return _lastTemp;
}

float AHT20::readHumidity() {
    float temp, hum;
    if (readData(&temp, &hum))
        return hum;
    return _lastHumidity;
}

} // namespace embedded