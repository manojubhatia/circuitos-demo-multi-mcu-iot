/**
 * @file LDRLight.hpp
 * @brief LDR (Light Dependent Resistor) Light Sensor

 */

#ifndef LDRLIGHT_HPP
#define LDRLIGHT_HPP

#include "base/Sensor.hpp"
#include "pal/pal_adc.h"

namespace embedded {

class LDRLight : public Sensor {
public:
    LDRLight(int adcPin, int adcUnit = 0);
    
    esp_err_t init() override;
    esp_err_t deinit() override;
    
    uint16_t readRaw();
    float readPercent();
    float readResistance(float seriesResistor = 10000.0f);

private:
    int _adcPin;
    int _adcUnit;
    int _adcChannel;
};

} // namespace embedded

#endif // LDRLIGHT_HPP