/**
 * @file INMP441.cpp
 * @brief INMP441 I2S MEMS Microphone Implementation

 */

#include "INMP441.hpp"
#include "osal/osal_log.h"
#include <cmath>
#include <cstring>
#include <cstdlib>

OSAL_LOG_MODULE_REGISTER(INMP441)

namespace embedded {

INMP441::INMP441(int i2sPort, int sckPin, int wsPin, int sdPin, int channel)
    : I2SSensor("INMP441", i2sPort, sckPin, wsPin, sdPin)
    , _i2sPort(i2sPort)
    , _sckPin(sckPin)
    , _wsPin(wsPin)
    , _sdPin(sdPin)
    , _channel(channel)
{
    memset(_readBuf, 0, sizeof(_readBuf));
}

INMP441::~INMP441() {
    deinit();
}

esp_err_t INMP441::init() {
    if (_initialized) {
        return ESP_OK;
    }
    
    _mutex = xSemaphoreCreateMutex();
    if (!_mutex) {
        OSAL_LOGE("Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t ret = _configureI2S();
    if (ret != ESP_OK) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
        return ret;
    }
    
    _initialized = true;
    OSAL_LOGI("Initialized: I2S%d, SCK=%d, WS=%d, SD=%d, SR=%lu Hz, %d-bit, Ch=%s, Gain=%.1f",
             _i2sPort, _sckPin, _wsPin, _sdPin, _sampleRate, _bitsPerSample,
             _channel == 0 ? "Left" : "Right", _gain);
    
    return ESP_OK;
}

esp_err_t INMP441::_configureI2S() {
    // ESP-IDF new I2S standard driver API
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        _i2sPort,
        I2S_ROLE_MASTER
    );
    chan_cfg.dma_desc_num = DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = DMA_BUF_LEN;
    
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &_rxChannel);
    if (ret != ESP_OK) {
        OSAL_LOGE("Failed to create I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)_sampleRate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            _bitsPerSample == 32 ? I2S_DATA_BIT_WIDTH_32BIT : I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_MONO
        ),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)_sckPin,
            .ws = (gpio_num_t)_wsPin,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)_sdPin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    if (_channel == 0) {
        std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    } else {
        std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    }
    
    ret = i2s_channel_init_std_mode(_rxChannel, &std_cfg);
    if (ret != ESP_OK) {
        OSAL_LOGE("Failed to init I2S std mode: %s", esp_err_to_name(ret));
        i2s_del_channel(_rxChannel);
        _rxChannel = nullptr;
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t INMP441::deinit() {
    if (!_initialized) {
        return ESP_OK;
    }
    
    stopRecording();
    
    if (_rxChannel) {
        i2s_channel_disable(_rxChannel);
        i2s_del_channel(_rxChannel);
        _rxChannel = nullptr;
    }
    
    if (_mutex) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
    
    _initialized = false;
    OSAL_LOGI("Deinitialized");
    return ESP_OK;
}

esp_err_t INMP441::setSampleRate(uint32_t sampleRate) {
    if (sampleRate < 8000 || sampleRate > 48000) {
        OSAL_LOGE("Invalid sample rate: %lu (valid: 8000-48000)", sampleRate);
        return ESP_ERR_INVALID_ARG;
    }
    
    bool wasRecording = _recording;
    if (wasRecording) stopRecording();
    
    if (_rxChannel) {
        i2s_channel_disable(_rxChannel);
        i2s_del_channel(_rxChannel);
        _rxChannel = nullptr;
    }
    
    _sampleRate = sampleRate;
    esp_err_t ret = _configureI2S();
    if (ret != ESP_OK) return ret;
    
    if (wasRecording) ret = startRecording();
    
    OSAL_LOGI("Sample rate set to %lu Hz", sampleRate);
    return ret;
}

esp_err_t INMP441::setBitsPerSample(int bits) {
    if (bits != 16 && bits != 32) {
        OSAL_LOGE("Invalid bits per sample: %d (valid: 16, 32)", bits);
        return ESP_ERR_INVALID_ARG;
    }
    
    bool wasRecording = _recording;
    if (wasRecording) stopRecording();
    
    if (_rxChannel) {
        i2s_channel_disable(_rxChannel);
        i2s_del_channel(_rxChannel);
        _rxChannel = nullptr;
    }
    
    _bitsPerSample = bits;
    esp_err_t ret = _configureI2S();
    if (ret != ESP_OK) return ret;
    
    if (wasRecording) ret = startRecording();
    
    OSAL_LOGI("Bits per sample set to %d", bits);
    return ret;
}

void INMP441::setGain(float gain) {
    if (gain < 0.1f) gain = 0.1f;
    if (gain > 10.0f) gain = 10.0f;
    _gain = gain;
}

esp_err_t INMP441::startRecording() {
    if (!_initialized) {
        OSAL_LOGE("Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (_recording) return ESP_OK;
    
    esp_err_t ret = i2s_channel_enable(_rxChannel);
    if (ret != ESP_OK) {
        OSAL_LOGE("Failed to enable I2S channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    _recording = true;
    
    if (_audioCallback) {
        BaseType_t task_ret = xTaskCreate(
            _readTaskFunc, "inmp441_read", 4096, this, 5, &_readTask
        );
        if (task_ret != pdPASS) {
            OSAL_LOGE("Failed to create read task");
            _recording = false;
            i2s_channel_disable(_rxChannel);
            return ESP_ERR_NO_MEM;
        }
    }
    
    OSAL_LOGI("Recording started");
    return ESP_OK;
}

void INMP441::stopRecording() {
    if (!_recording) return;
    
    _recording = false;
    
    if (_readTask) {
        vTaskDelay(pdMS_TO_TICKS(100));
        _readTask = nullptr;
    }
    
    if (_rxChannel) {
        i2s_channel_disable(_rxChannel);
    }
    
    OSAL_LOGI("Recording stopped");
}

void INMP441::_readTaskFunc(void* arg) {
    INMP441* mic = static_cast<INMP441*>(arg);
    
    int16_t* buffer = (int16_t*)heap_caps_malloc(READ_BUF_SAMPLES * sizeof(int16_t), MALLOC_CAP_DMA);
    if (!buffer) {
        OSAL_LOGE("Failed to allocate read buffer");
        vTaskDelete(nullptr);
        return;
    }
    
    while (mic->_recording) {
        size_t samplesRead = 0;
        esp_err_t ret = mic->read(buffer, READ_BUF_SAMPLES, &samplesRead, 100);
        
        if (ret == ESP_OK && samplesRead > 0) {
            mic->_updateMetrics(buffer, samplesRead);
            if (mic->_audioCallback) {
                mic->_audioCallback(buffer, samplesRead, mic->_audioUserData);
            }
        }
    }
    
    free(buffer);
    vTaskDelete(nullptr);
}

esp_err_t INMP441::read(int16_t* buffer, size_t maxSamples, size_t* samplesRead, uint32_t timeoutMs) {
    if (!_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!buffer || maxSamples == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t bytesRead = 0;
    esp_err_t ret;
    
    if (!_rxChannel) return ESP_ERR_INVALID_STATE;
    
    if (_bitsPerSample == 32) {
        int32_t* tempBuffer = (int32_t*)heap_caps_malloc(maxSamples * sizeof(int32_t), MALLOC_CAP_DMA);
        if (!tempBuffer) return ESP_ERR_NO_MEM;
        
        ret = i2s_channel_read(_rxChannel, tempBuffer, maxSamples * sizeof(int32_t),
                               &bytesRead, pdMS_TO_TICKS(timeoutMs));
        
        if (ret == ESP_OK) {
            size_t samples = bytesRead / sizeof(int32_t);
            for (size_t i = 0; i < samples; i++) {
                int32_t sample = (int32_t)((tempBuffer[i] >> 16) * _gain);
                if (sample > 32767) sample = 32767;
                if (sample < -32768) sample = -32768;
                buffer[i] = (int16_t)sample;
            }
            *samplesRead = samples;
        }
        free(tempBuffer);
    } else {
        ret = i2s_channel_read(_rxChannel, buffer, maxSamples * sizeof(int16_t),
                               &bytesRead, pdMS_TO_TICKS(timeoutMs));
        if (ret == ESP_OK) {
            *samplesRead = bytesRead / sizeof(int16_t);
            if (_gain != 1.0f) {
                for (size_t i = 0; i < *samplesRead; i++) {
                    int32_t sample = (int32_t)(buffer[i] * _gain);
                    if (sample > 32767) sample = 32767;
                    if (sample < -32768) sample = -32768;
                    buffer[i] = (int16_t)sample;
                }
            }
        }
    }
    
    return ret;
}

esp_err_t INMP441::readRaw(int32_t* buffer, size_t maxSamples, size_t* samplesRead, uint32_t timeoutMs) {
    if (!_initialized) return ESP_ERR_INVALID_STATE;
    if (!buffer || maxSamples == 0) return ESP_ERR_INVALID_ARG;
    
    size_t bytesRead = 0;
    if (!_rxChannel) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = i2s_channel_read(_rxChannel, buffer, maxSamples * sizeof(int32_t),
                                     &bytesRead, pdMS_TO_TICKS(timeoutMs));
    if (ret == ESP_OK) {
        *samplesRead = bytesRead / sizeof(int32_t);
    }
    return ret;
}

void INMP441::setAudioCallback(AudioCallback callback, void* userData) {
    _audioCallback = callback;
    _audioUserData = userData;
}

// ── Simple read methods for sensor task polling ──

float INMP441::read() {
    return readAudioLevel();
}

float INMP441::readAudioLevel() {
    if (!_recording) {
        startRecording();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    size_t samplesRead = 0;
    esp_err_t ret = read(_readBuf, READ_BUF_SAMPLES, &samplesRead, 100);
    if (ret == ESP_OK && samplesRead > 0) {
        _updateMetrics(_readBuf, samplesRead);
    }
    
    xSemaphoreTake(_mutex, portMAX_DELAY);
    float db = _currentDb;
    xSemaphoreGive(_mutex);
    return db;
}

float INMP441::readRmsLevel() {
    if (!_recording) {
        startRecording();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    size_t samplesRead = 0;
    esp_err_t ret = read(_readBuf, READ_BUF_SAMPLES, &samplesRead, 100);
    if (ret == ESP_OK && samplesRead > 0) {
        _updateMetrics(_readBuf, samplesRead);
    }
    
    xSemaphoreTake(_mutex, portMAX_DELAY);
    float rms = _currentRMS;
    xSemaphoreGive(_mutex);
    return rms;
}

int INMP441::readPeakAmplitude() {
    if (!_recording) {
        startRecording();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    size_t samplesRead = 0;
    esp_err_t ret = read(_readBuf, READ_BUF_SAMPLES, &samplesRead, 100);
    if (ret == ESP_OK && samplesRead > 0) {
        _updateMetrics(_readBuf, samplesRead);
    }
    
    xSemaphoreTake(_mutex, portMAX_DELAY);
    int peak = _currentPeak;
    xSemaphoreGive(_mutex);
    return peak;
}

bool INMP441::readIsClipping() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    bool clipping = _currentClipping;
    xSemaphoreGive(_mutex);
    return clipping;
}

void INMP441::_updateMetrics(const int16_t* buffer, size_t numSamples) {
    float rms = calculateRMS(buffer, numSamples);
    float db = rmsToDb(rms);
    int16_t peak = getPeakAmplitude(buffer, numSamples);
    bool clip = isClipping(buffer, numSamples);
    
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _currentRMS = rms;
    _currentDb = db;
    _currentPeak = peak;
    _currentClipping = clip;
    xSemaphoreGive(_mutex);
}

float INMP441::getAudioLevelDb() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    float db = _currentDb;
    xSemaphoreGive(_mutex);
    return db;
}

float INMP441::getRMSLevel() {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    float rms = _currentRMS;
    xSemaphoreGive(_mutex);
    return rms;
}

// ── Static utility methods ──

float INMP441::calculateRMS(const int16_t* buffer, size_t numSamples) {
    if (!buffer || numSamples == 0) return 0.0f;
    
    double sumSquares = 0.0;
    for (size_t i = 0; i < numSamples; i++) {
        double sample = buffer[i] / 32768.0;
        sumSquares += sample * sample;
    }
    return sqrtf((float)(sumSquares / numSamples));
}

float INMP441::rmsToDb(float rms) {
    if (rms <= 0.0f) return -96.0f;
    return 20.0f * log10f(rms);
}

int16_t INMP441::getPeakAmplitude(const int16_t* buffer, size_t numSamples) {
    if (!buffer || numSamples == 0) return 0;
    
    int16_t peak = 0;
    for (size_t i = 0; i < numSamples; i++) {
        int16_t abs_val = buffer[i] < 0 ? -buffer[i] : buffer[i];
        if (abs_val > peak) peak = abs_val;
    }
    return peak;
}

bool INMP441::isClipping(const int16_t* buffer, size_t numSamples, int16_t threshold) {
    if (!buffer || numSamples == 0) return false;
    
    for (size_t i = 0; i < numSamples; i++) {
        int16_t abs_val = buffer[i] < 0 ? -buffer[i] : buffer[i];
        if (abs_val >= threshold) return true;
    }
    return false;
}

} // namespace embedded