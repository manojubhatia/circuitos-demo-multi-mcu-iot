/**
 * @file RGBLed.hpp
 * @brief RGB LED (Common Cathode/Anode) with PWM

 */

#ifndef RGBLED_HPP
#define RGBLED_HPP

#include "base/Actuator.hpp"
#include "pal/pal_pwm.h"
#include "driver/ledc.h"

namespace embedded {

class RGBLed : public Actuator {
public:
    RGBLed(int redPin, int greenPin, int bluePin, bool commonAnode = false);
    
    esp_err_t init() override;
    esp_err_t deinit() override;
    
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setColorHex(uint32_t hex);
    void setColorHSV(float h, float s, float v);
    void off();
    void setBrightness(uint8_t brightness);

private:
    int _redPin, _greenPin, _bluePin;
    bool _commonAnode;
    uint8_t _brightness = 255;
    
    void setChannelDuty(ledc_channel_t channel, uint8_t value);
};

} // namespace embedded

#endif // RGBLED_HPP