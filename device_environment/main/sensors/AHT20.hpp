/**
 * @file AHT20.hpp
 * @brief AHT20 Temperature and Humidity Sensor

 */

#ifndef AHT20_HPP
#define AHT20_HPP

#include "base/Sensor.hpp"
#include "pal/pal_i2c.h"

namespace embedded {

class AHT20 : public Sensor {
public:
    // addr: I2C address (0x38 default), sdaPin: SDA GPIO, sclPin: SCL GPIO
    AHT20(uint8_t addr = 0x38, int sdaPin = 21, int sclPin = 22);
    
    esp_err_t init() override;
    esp_err_t deinit() override;
    
    float readTemperature();
    float readHumidity();
    float read();
    bool readData(float* temp, float* humidity);

    /** @brief Get last successfully read temperature (no I2C transaction) */
    float getLastTemperature() const { return _lastTemp; }
    /** @brief Get last successfully read humidity (no I2C transaction) */
    float getLastHumidity() const { return _lastHumidity; }

private:
    uint8_t _address;
    int _sdaPin, _sclPin;
    int _i2cPort = 0;
    float _lastTemp = 0.0f;
    float _lastHumidity = 0.0f;
    
    bool triggerMeasurement();
    bool readRawData(uint8_t* data);
};

} // namespace embedded

#endif // AHT20_HPP