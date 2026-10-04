/**
 * @file Buzzer.cpp
 * @brief Piezo Buzzer Implementation (PAL-based)

 */

#include "Buzzer.hpp"
#include "pal/pal_gpio.h"
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_log.h"

OSAL_LOG_MODULE_REGISTER(Buzzer)

namespace embedded {

Buzzer::Buzzer(int pin, bool isPassive, int pwmTimer, int pwmChannel)
    : Actuator("Buzzer"), _pin(pin), _isPassive(isPassive),
      _pwmTimer(pwmTimer), _pwmChannel(pwmChannel)
{
}

esp_err_t Buzzer::init() {
    if (_initialized) return ESP_OK;
    
    if (_isPassive) {
        int ret = pal_pwm_init(_pin, 2000, 10);
        if (ret != 0) return ESP_FAIL;
        pal_pwm_set_duty(_pin, 0);
    } else {
        pal_gpio_init(_pin, PAL_GPIO_OUTPUT);
        pal_gpio_set(_pin, 0);
    }
    
    OSAL_LOGI("Initialized on GPIO%d (%s)", _pin, _isPassive ? "passive" : "active");
    _initialized = true;
    return ESP_OK;
}

esp_err_t Buzzer::deinit() {
    noTone();
    _initialized = false;
    return ESP_OK;
}

void Buzzer::tone(uint32_t frequency, uint32_t duration_ms) {
    if (!_initialized) return;
    
    if (_isPassive) {
        if (frequency == 0) {
            noTone();
            return;
        }
        pal_pwm_set_frequency(_pin, frequency);
        pal_pwm_set_duty_percent(_pin, 50.0f);
        _playing = true;
        
        if (duration_ms > 0) {
            osal_task_delay_ms(duration_ms);
            noTone();
        }
    } else {
        pal_gpio_set(_pin, 1);
        _playing = true;
        
        if (duration_ms > 0) {
            osal_task_delay_ms(duration_ms);
            noTone();
        }
    }
}

void Buzzer::noTone() {
    if (_isPassive) {
        pal_pwm_set_duty(_pin, 0);
    } else {
        pal_gpio_set(_pin, 0);
    }
    _playing = false;
}

void Buzzer::beep(uint32_t duration_ms, uint32_t frequency) {
    tone(frequency, duration_ms);
}

void Buzzer::playMelody(const uint16_t* notes, const uint16_t* durations, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (notes[i] == 0) {
            noTone();
            osal_task_delay_ms(durations[i]);
        } else {
            tone(notes[i], durations[i]);
        }
        osal_task_delay_ms(50);
    }
    noTone();
}

} // namespace embedded