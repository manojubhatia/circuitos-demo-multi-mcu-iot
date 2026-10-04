/**
 * @file LDRLight.cpp
 * @brief LDR Light Sensor Implementation (PAL-based)

 */

#include "LDRLight.hpp"
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_log.h"

OSAL_LOG_MODULE_REGISTER(LDRLight)

namespace embedded {

LDRLight::LDRLight(int adcPin, int adcUnit)
    : Sensor("LDRLight"), _adcPin(adcPin), _adcUnit(adcUnit), _adcChannel(0)
{
}

esp_err_t LDRLight::init() {
    if (_initialized) return ESP_OK;

    int ret = pal_adc_init(_adcPin, PAL_ADC_ATTEN_11DB, PAL_ADC_WIDTH_12BIT);
    if (ret != 0) {
        OSAL_LOGE("Failed to init ADC on pin %d", _adcPin);
        return ESP_FAIL;
    }

    OSAL_LOGI("Initialized on GPIO%d", _adcPin);
    _initialized = true;
    return ESP_OK;
}

esp_err_t LDRLight::deinit() {
    _initialized = false;
    return ESP_OK;
}

uint16_t LDRLight::readRaw() {
    if (!_initialized) return 0;
    return (uint16_t)pal_adc_read_raw(_adcPin);
}

float LDRLight::readPercent() {
    return (readRaw() / 4095.0f) * 100.0f;
}

float LDRLight::readResistance(float seriesResistor) {
    uint16_t raw = readRaw();
    if (raw == 0) return 1000000.0f;
    float voltage = (raw / 4095.0f) * 3.3f;
    return seriesResistor * (3.3f - voltage) / voltage;
}

} // namespace embedded