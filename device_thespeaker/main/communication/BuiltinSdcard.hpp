/**
 * @file BuiltinSdcard.hpp
 * @brief Stub header for MicroSD Card Slot
 * 
 * This component does not have a template yet.
 * Implement the methods below based on the component datasheet.
 * 
 * Interface: SPI
 * Registry ID: builtin-sdcard
 */

#ifndef BUILTINSDCARD_HPP
#define BUILTINSDCARD_HPP

#include <cstdint>
#include "osal/osal_types.h"
#include "osal/osal_log.h"

namespace embedded {

/**
 * @brief Stub class for MicroSD Card Slot
 */
class BuiltinSdcard {
public:
    BuiltinSdcard(int csPin = -1, int dcPin = -1, int rstPin = -1, int mosiPin = -1, int sclkPin = -1, int blPin = -1) : m_csPin(csPin), m_dcPin(dcPin), m_rstPin(rstPin), m_mosiPin(mosiPin), m_sclkPin(sclkPin), m_blPin(blPin), m_initialized(false) {}
    
    ~BuiltinSdcard() {
        if (m_initialized) {
            deinit();
        }
    }
    
    esp_err_t init() {
        m_initialized = true;
        return ESP_OK;
    }
    
    esp_err_t deinit() {
        m_initialized = false;
        return ESP_OK;
    }
    
    void update() {}
    void poll() {}
    
    float readRaw() { return 0.0f; }
    float readPercent() { return 0.0f; }
    bool isTouched() { return false; }
    float count() { return 0.0f; }
    float start() { return 0.0f; }
    float end() { return 0.0f; }
    float audio_samples() { return 0.0f; }
    
    void clear(uint16_t color) {}
    
    bool isReady() const { return m_initialized; }
    bool isInitialized() const { return m_initialized; }

private:
    int m_csPin;
    int m_dcPin;
    int m_rstPin;
    int m_mosiPin;
    int m_sclkPin;
    int m_blPin;
    bool m_initialized;
};

} // namespace embedded

#endif // BUILTINSDCARD_HPP
