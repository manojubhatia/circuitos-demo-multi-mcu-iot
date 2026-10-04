/**
 * @file PAM8403.cpp
 * @brief PAM8403 Stereo Class-D Audio Amplifier Implementation (PAL-based)
 * @note Auto-generated - Customize as needed
 */

#include "PAM8403.hpp"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "osal/osal_task.h"

OSAL_LOG_MODULE_REGISTER(PAM8403)

namespace embedded {

PAM8403::PAM8403(int leftInputPin, int rightInputPin,
                                   int mutePin, int shutdownPin)
    : Actuator("PAM8403")
    , _leftPin(leftInputPin)
    , _rightPin(rightInputPin)
    , _mutePin(mutePin)
    , _shutdownPin(shutdownPin)
    , _enabled(false)
    , _muted(false)
    , _sampleRate(44100)
    , _bitDepth(8)
    , _maxValue(255)
{
}

PAM8403::~PAM8403() {
    deinit();
}

esp_err_t PAM8403::init() {
    return initAudio() ? ESP_OK : ESP_FAIL;
}

esp_err_t PAM8403::deinit() {
    if (!_initialized) return ESP_OK;
    setMute(true);
    setEnabled(false);
    pal_pwm_stop(_leftPin);
    pal_pwm_stop(_rightPin);
    _initialized = false;
    return ESP_OK;
}

bool PAM8403::initAudio(uint32_t sampleRate, uint8_t bitDepth) {
    if (_initialized) return true;

    _sampleRate = sampleRate;
    _bitDepth = bitDepth;
    _maxValue = (1 << bitDepth) - 1;

    OSAL_LOGI("Initializing PAM8403 amplifier");
    OSAL_LOGI("  Left input pin: GPIO%d", _leftPin);
    OSAL_LOGI("  Right input pin: GPIO%d", _rightPin);
    OSAL_LOGI("  Sample rate: %lu Hz", _sampleRate);
    OSAL_LOGI("  Bit depth: %d bits", _bitDepth);

    if (_mutePin >= 0) {
        pal_gpio_init(_mutePin, PAL_GPIO_OUTPUT);
        pal_gpio_set(_mutePin, true);
        OSAL_LOGI("  Mute pin: GPIO%d", _mutePin);
    }

    if (_shutdownPin >= 0) {
        pal_gpio_init(_shutdownPin, PAL_GPIO_OUTPUT);
        pal_gpio_set(_shutdownPin, true);
        OSAL_LOGI("  Shutdown pin: GPIO%d", _shutdownPin);
    }

    if (!configurePWM()) {
        OSAL_LOGE("Failed to configure PWM for audio output");
        return false;
    }

    _enabled = true;
    _muted = false;
    _initialized = true;

    OSAL_LOGI("PAM8403 initialized successfully");
    return true;
}

bool PAM8403::configurePWM() {
    int ret = pal_pwm_init(_leftPin, 40000, _bitDepth);
    if (ret != 0) {
        OSAL_LOGE("Failed to configure left channel PWM");
        return false;
    }
    ret = pal_pwm_init(_rightPin, 40000, _bitDepth);
    if (ret != 0) {
        OSAL_LOGE("Failed to configure right channel PWM");
        return false;
    }
    OSAL_LOGI("PWM configured: 40kHz, %d-bit resolution", _bitDepth);
    return true;
}

void PAM8403::setEnabled(bool enable) {
    if (!_initialized) return;
    _enabled = enable;
    setShutdownPin(!enable);
    OSAL_LOGI("Amplifier %s", enable ? "enabled" : "disabled");
}

void PAM8403::setMute(bool mute) {
    if (!_initialized) return;
    _muted = mute;
    setMutePin(mute);
    OSAL_LOGI("Amplifier %s", mute ? "muted" : "unmuted");
}

void PAM8403::setMutePin(bool mute) {
    if (_mutePin < 0) return;
    pal_gpio_set(_mutePin, mute ? false : true);
}

void PAM8403::setShutdownPin(bool shutdown) {
    if (_shutdownPin < 0) return;
    pal_gpio_set(_shutdownPin, shutdown ? false : true);
}

void PAM8403::setOutput(AudioChannel channel, uint16_t value) {
    if (!_initialized || !_enabled || _muted) return;
    if (value > _maxValue) value = _maxValue;
    float pct = (float)value / (float)_maxValue * 100.0f;

    switch (channel) {
        case AudioChannel::LEFT:
            pal_pwm_set_duty_percent(_leftPin, pct);
            break;
        case AudioChannel::RIGHT:
            pal_pwm_set_duty_percent(_rightPin, pct);
            break;
        case AudioChannel::BOTH:
            pal_pwm_set_duty_percent(_leftPin, pct);
            pal_pwm_set_duty_percent(_rightPin, pct);
            break;
    }
}

void PAM8403::setStereoOutput(uint16_t leftValue, uint16_t rightValue) {
    if (!_initialized || !_enabled || _muted) return;
    if (leftValue > _maxValue) leftValue = _maxValue;
    if (rightValue > _maxValue) rightValue = _maxValue;
    pal_pwm_set_duty_percent(_leftPin, (float)leftValue / (float)_maxValue * 100.0f);
    pal_pwm_set_duty_percent(_rightPin, (float)rightValue / (float)_maxValue * 100.0f);
}

void PAM8403::setOutputPercent(AudioChannel channel, float percent) {
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 1.0f) percent = 1.0f;
    uint16_t value = (uint16_t)(percent * _maxValue);
    setOutput(channel, value);
}

void PAM8403::playTone(AudioChannel channel, uint32_t frequencyHz, uint8_t volume) {
    if (!_initialized || !_enabled) return;
    float duty = (float)volume / 255.0f * 50.0f;

    switch (channel) {
        case AudioChannel::LEFT:
            pal_pwm_set_frequency(_leftPin, frequencyHz);
            pal_pwm_set_duty_percent(_leftPin, duty);
            break;
        case AudioChannel::RIGHT:
            pal_pwm_set_frequency(_rightPin, frequencyHz);
            pal_pwm_set_duty_percent(_rightPin, duty);
            break;
        case AudioChannel::BOTH:
            pal_pwm_set_frequency(_leftPin, frequencyHz);
            pal_pwm_set_duty_percent(_leftPin, duty);
            pal_pwm_set_frequency(_rightPin, frequencyHz);
            pal_pwm_set_duty_percent(_rightPin, duty);
            break;
    }
    OSAL_LOGD("Playing tone: %lu Hz, volume: %d", frequencyHz, volume);
}

void PAM8403::stopTone(AudioChannel channel) {
    if (!_initialized) return;

    switch (channel) {
        case AudioChannel::LEFT:
            pal_pwm_set_duty_percent(_leftPin, 0.0f);
            break;
        case AudioChannel::RIGHT:
            pal_pwm_set_duty_percent(_rightPin, 0.0f);
            break;
        case AudioChannel::BOTH:
            pal_pwm_set_duty_percent(_leftPin, 0.0f);
            pal_pwm_set_duty_percent(_rightPin, 0.0f);
            break;
    }
    configurePWM();
}

void PAM8403::beep(uint32_t frequencyHz, uint32_t durationMs, uint8_t volume) {
    playTone(AudioChannel::BOTH, frequencyHz, volume);
    osal_task_delay_ms(durationMs);
    stopTone(AudioChannel::BOTH);
}

uint16_t PAM8403::sampleToPWM(int16_t sample) {
    int32_t shifted = sample + 32768;
    return (uint16_t)((shifted * _maxValue) / 65535);
}

void PAM8403::writeSample(int16_t leftSample, int16_t rightSample) {
    if (!_initialized || !_enabled || _muted) return;
    uint16_t leftPWM = sampleToPWM(leftSample);
    uint16_t rightPWM = sampleToPWM(rightSample);
    setStereoOutput(leftPWM, rightPWM);
}

void PAM8403::writeMonoSample(int16_t sample) {
    writeSample(sample, sample);
}

} // namespace embedded