/**
 * @file INMP441.hpp
 * @brief INMP441 I2S MEMS Microphone Sensor

 */

#ifndef INMP441_HPP
#define INMP441_HPP

#include "base/Sensor.hpp"
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_sync.h"

#if __has_include("pal/pal_i2s.h")
#include "pal/pal_i2s.h"
#endif

#if defined(ESP_PLATFORM)
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#endif

namespace embedded {

class INMP441 : public I2SSensor {
public:
    typedef void (*AudioCallback)(const int16_t* samples, size_t num_samples, void* user_data);

    INMP441(int i2sPort, int sckPin, int wsPin, int sdPin, int channel = 0);
    ~INMP441();
    
    esp_err_t init() override;
    esp_err_t deinit() override;
    
    esp_err_t setSampleRate(uint32_t sampleRate);
    uint32_t getSampleRate() const { return _sampleRate; }
    
    esp_err_t setBitsPerSample(int bits);
    int getBitsPerSample() const { return _bitsPerSample; }
    
    void setGain(float gain);
    float getGain() const { return _gain; }
    
    esp_err_t startRecording();
    void stopRecording();
    bool isRecording() const { return _recording; }
    
    /**
     * @brief Read audio samples (blocking)
     * @param buffer Output buffer for samples
     * @param maxSamples Maximum samples to read
     * @param samplesRead Actual samples read
     * @param timeoutMs Read timeout in milliseconds
     * @return ESP_OK on success
     */
    esp_err_t read(int16_t* buffer, size_t maxSamples, size_t* samplesRead, uint32_t timeoutMs);
    
    esp_err_t readRaw(int32_t* buffer, size_t maxSamples, size_t* samplesRead, uint32_t timeoutMs);
    
    void setAudioCallback(AudioCallback callback, void* userData);
    i2s_chan_handle_t getRxChannel() const { return _rxChannel; }
    
    // ── Simple read methods matching variable naming conventions ──
    // These do a blocking read internally and return computed values.
    // Suitable for periodic polling from a sensor task.
    
    float read();  // Default: returns audio level in dB (alias for readAudioLevel)
    float readAudioLevel();
    float audio_samples() { return readAudioLevel(); }  // Signal-name alias
    float readRmsLevel();
    int readPeakAmplitude();
    bool readIsClipping();
    
    // ── Level accessors (updated by background task or readXxx methods) ──
    
    float getAudioLevelDb();
    float getRMSLevel();
    
    // ── Static utility methods ──
    
    static float calculateRMS(const int16_t* buffer, size_t numSamples);
    static float rmsToDb(float rms);
    static int16_t getPeakAmplitude(const int16_t* buffer, size_t numSamples);
    static bool isClipping(const int16_t* buffer, size_t numSamples, int16_t threshold = 32000);

private:
    int _i2sPort;
    int _sckPin;
    int _wsPin;
    int _sdPin;
    int _channel;
    
    uint32_t _sampleRate = 16000;
    int _bitsPerSample = 16;
    float _gain = 1.0f;
    
    i2s_chan_handle_t _rxChannel = nullptr;
    bool _recording = false;
    
    AudioCallback _audioCallback = nullptr;
    void* _audioUserData = nullptr;
    
    TaskHandle_t _readTask = nullptr;
    SemaphoreHandle_t _mutex = nullptr;
    
    float _currentRMS = 0;
    float _currentDb = -96.0f;
    int16_t _currentPeak = 0;
    bool _currentClipping = false;
    
    static const int DMA_BUF_COUNT = 8;
    static const int DMA_BUF_LEN = 64;
    static const int READ_BUF_SAMPLES = 512;
    
    int16_t _readBuf[READ_BUF_SAMPLES];
    
    esp_err_t _configureI2S();
    void _updateMetrics(const int16_t* buffer, size_t numSamples);
    static void _readTaskFunc(void* arg);
};

} // namespace embedded

#endif // INMP441_HPP