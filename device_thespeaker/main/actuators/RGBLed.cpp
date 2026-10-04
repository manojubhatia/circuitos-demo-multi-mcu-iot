/**
 * @file RGBLed.cpp
 * @brief RGB LED Implementation

 */

#include "RGBLed.hpp"
#include "driver/ledc.h"
#include "osal/osal_log.h"
#include <math.h>

static const char* TAG = "RGBLed";
OSAL_LOG_MODULE_REGISTER(RGBLed);

namespace embedded {

RGBLed::RGBLed(int redPin, int greenPin, int bluePin, bool commonAnode)
    : Actuator("RGBLed"), _redPin(redPin), _greenPin(greenPin),
      _bluePin(bluePin), _commonAnode(commonAnode)
{
}

esp_err_t RGBLed::init() {
    if (_initialized) return ESP_OK;
    
    // Configure timer
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
    timer_conf.timer_num = LEDC_TIMER_3;
    timer_conf.freq_hz = 5000;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) return ret;
    
    // Configure channels
    const int pins[3] = { _redPin, _greenPin, _bluePin };
    const ledc_channel_t chans[3] = { LEDC_CHANNEL_3, LEDC_CHANNEL_4, LEDC_CHANNEL_5 };
    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t ch_conf = {};
        ch_conf.gpio_num = pins[i];
        ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
        ch_conf.channel = chans[i];
        ch_conf.timer_sel = LEDC_TIMER_3;
        ch_conf.duty = 0;
        ch_conf.hpoint = 0;
        ret = ledc_channel_config(&ch_conf);
        if (ret != ESP_OK) return ret;
    }
    
    off();
    
    OSAL_LOGI("Initialized (R: GPIO%d, G: GPIO%d, B: GPIO%d)", _redPin, _greenPin, _bluePin);
    _initialized = true;
    return ESP_OK;
}

esp_err_t RGBLed::deinit() {
    off();
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0);
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_4, 0);
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_5, 0);
    _initialized = false;
    return ESP_OK;
}

void RGBLed::setChannelDuty(ledc_channel_t channel, uint8_t value) {
    uint8_t adjustedValue = (value * _brightness) >> 8;
    if (_commonAnode) {
        adjustedValue = 255 - adjustedValue;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, adjustedValue);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void RGBLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!_initialized) return;
    setChannelDuty(LEDC_CHANNEL_3, r);
    setChannelDuty(LEDC_CHANNEL_4, g);
    setChannelDuty(LEDC_CHANNEL_5, b);
}

void RGBLed::setColorHex(uint32_t hex) {
    setColor((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

void RGBLed::setColorHSV(float h, float s, float v) {
    float r, g, b;
    int i = (int)floorf(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    
    setColor((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

void RGBLed::off() {
    setColor(0, 0, 0);
}

void RGBLed::setBrightness(uint8_t brightness) {
    _brightness = brightness;
}

} // namespace embedded