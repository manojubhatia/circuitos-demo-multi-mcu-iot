/**
 * @file PAM8403.hpp
 * @brief PAM8403 Stereo Class-D Audio Amplifier Driver
 * 
 * The PAM8403 is a 3W+3W stereo Class-D audio amplifier with:
 * - 2.5V to 5.5V operation
 * - Stereo BTL (Bridge-Tied Load) output
 * - 90% efficiency at 10% THD+N
 * - Optional MUTE and SHDN control pins
 * 
 * Wiring:
 *   VCC  -> 5V (or 3.3V for lower power)
 *   GND  -> Ground
 *   LIN  -> Left audio input (from DAC/PWM with filter)
 *   RIN  -> Right audio input (from DAC/PWM with filter)
 *   LOUT+/LOUT- -> Left speaker (BTL, do NOT ground)
 *   ROUT+/ROUT- -> Right speaker (BTL, do NOT ground)
 *   MUTE -> Optional mute control (active LOW)
 *   SHDN -> Optional shutdown control (active LOW)
 * 
 * @note Auto-generated - Customize as needed
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "base/Actuator.hpp"
#include "osal/osal_types.h"
#include "pal/pal_pwm.h"
#include "pal/pal_gpio.h"
#if __has_include("driver/dac_oneshot.h")
#include "pal/pal_adc.h"
#define HAS_DAC 1
#elif __has_include("driver/dac.h")
#include "pal/pal_adc.h"
#define HAS_DAC 1
#else
#define HAS_DAC 0
#endif

namespace embedded {

/**
 * @brief Audio channel enumeration
 */
enum class AudioChannel : uint8_t {
    LEFT = 0,
    RIGHT = 1,
    BOTH = 2
};

/**
 * @brief PAM8403 Stereo Amplifier Driver
 * 
 * Supports both PWM-based audio output and DAC output (if available).
 * Uses LEDC PWM peripheral on ESP32 for high-quality audio.
 */
class PAM8403 : public Actuator {
public:
    /**
     * @brief Construct PAM8403 driver
     * @param leftInputPin GPIO for left channel audio input (PWM output)
     * @param rightInputPin GPIO for right channel audio input (PWM output)
     * @param mutePin Optional mute control pin (-1 to disable)
     * @param shutdownPin Optional shutdown control pin (-1 to disable)
     */
    PAM8403(int leftInputPin, int rightInputPin, 
                     int mutePin = -1, int shutdownPin = -1);
    
    ~PAM8403();
    
    esp_err_t init() override;
    esp_err_t deinit() override;
    
    /**
     * @brief Initialize the amplifier with audio parameters
     * @param sampleRate Audio sample rate in Hz (default 44100)
     * @param bitDepth PWM resolution in bits (default 8)
     * @return true on success
     */
    bool initAudio(uint32_t sampleRate = 44100, uint8_t bitDepth = 8);
    
    // isInitialized() and update() inherited from Actuator base class
    
    // ═══════════════════════════════════════════════════════════════════════
    // AMPLIFIER CONTROL
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Enable or disable the amplifier
     * @param enable true to enable, false to shutdown
     */
    void setEnabled(bool enable);
    
    /**
     * @brief Check if amplifier is enabled
     */
    bool isEnabled() const { return _enabled; }
    
    /**
     * @brief Mute or unmute the amplifier
     * @param mute true to mute, false to unmute
     */
    void setMute(bool mute);
    
    /**
     * @brief Check if muted
     */
    bool isMuted() const { return _muted; }
    
    // ═══════════════════════════════════════════════════════════════════════
    // AUDIO OUTPUT - DIRECT VALUE
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Set audio output level for a channel (0-255 for 8-bit)
     * @param channel Audio channel (LEFT, RIGHT, or BOTH)
     * @param value Output value (0 to max based on bit depth)
     */
    void setOutput(AudioChannel channel, uint16_t value);
    
    /**
     * @brief Set audio output for both channels
     * @param leftValue Left channel value
     * @param rightValue Right channel value
     */
    void setStereoOutput(uint16_t leftValue, uint16_t rightValue);
    
    /**
     * @brief Set output as percentage (0.0 to 1.0)
     * @param channel Audio channel
     * @param percent Output level (0.0 = silence, 1.0 = max)
     */
    void setOutputPercent(AudioChannel channel, float percent);
    
    // ═══════════════════════════════════════════════════════════════════════
    // TONE GENERATION
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Play a tone at specified frequency
     * @param channel Audio channel
     * @param frequencyHz Tone frequency in Hz
     * @param volume Volume level (0-255)
     */
    void playTone(AudioChannel channel, uint32_t frequencyHz, uint8_t volume = 128);
    
    /**
     * @brief Stop tone playback
     * @param channel Audio channel to stop
     */
    void stopTone(AudioChannel channel);
    
    /**
     * @brief Play a beep for specified duration
     * @param frequencyHz Tone frequency
     * @param durationMs Duration in milliseconds
     * @param volume Volume level
     */
    void beep(uint32_t frequencyHz, uint32_t durationMs, uint8_t volume = 128);
    
    // ═══════════════════════════════════════════════════════════════════════
    // AUDIO BUFFER PLAYBACK
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Write audio sample to output buffer
     * @param leftSample Left channel sample (signed 16-bit)
     * @param rightSample Right channel sample (signed 16-bit)
     * @note Converts to PWM duty cycle internally
     */
    void writeSample(int16_t leftSample, int16_t rightSample);
    
    /**
     * @brief Write mono sample to both channels
     * @param sample Audio sample (signed 16-bit)
     */
    void writeMonoSample(int16_t sample);
    
    // ═══════════════════════════════════════════════════════════════════════
    // CONFIGURATION
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * @brief Get configured sample rate
     */
    uint32_t getSampleRate() const { return _sampleRate; }
    
    /**
     * @brief Get configured bit depth
     */
    uint8_t getBitDepth() const { return _bitDepth; }
    
    /**
     * @brief Get maximum output value based on bit depth
     */
    uint16_t getMaxValue() const { return (1 << _bitDepth) - 1; }

private:
    // Pin configuration
    int _leftPin;
    int _rightPin;
    int _mutePin;
    int _shutdownPin;
    
    // State (_initialized is in Actuator base)
    bool _enabled;
    bool _muted;
    
    // Audio configuration
    uint32_t _sampleRate;
    uint8_t _bitDepth;
    uint16_t _maxValue;
    
    bool configurePWM();
    
    // Internal helpers
    void setMutePin(bool mute);
    void setShutdownPin(bool shutdown);
    uint16_t sampleToPWM(int16_t sample);
};

} // namespace embedded