/**
 * @file Buzzer.hpp
 * @brief Piezo Buzzer (Active or Passive)

 */

#ifndef BUZZER_HPP
#define BUZZER_HPP

#include "base/Actuator.hpp"
#include "pal/pal_pwm.h"

namespace embedded {

class Buzzer : public Actuator {
public:
    Buzzer(int pin, bool isPassive = true,
                     int pwmTimer = 1, int pwmChannel = 1);
    
    esp_err_t init() override;
    esp_err_t deinit() override;
    
    void tone(uint32_t frequency, uint32_t duration_ms = 0);
    void noTone();
    void beep(uint32_t duration_ms = 100, uint32_t frequency = 2000);
    void playMelody(const uint16_t* notes, const uint16_t* durations, size_t length);

private:
    int _pin;
    bool _isPassive;
    int _pwmTimer;
    int _pwmChannel;
    bool _playing = false;
};

} // namespace embedded

#endif // BUZZER_HPP