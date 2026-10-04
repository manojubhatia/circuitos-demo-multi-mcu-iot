/**
 * @file hal_adc_esp.c
 * @brief Hardware Abstraction Layer - ADC ESP-IDF Implementation

 */

#include "hal_adc.h"

#if HAL_PLATFORM_ESP_IDF

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "hal_adc";

/* ============================================================================
 * Internal State
 * ============================================================================ */

typedef struct {
    bool configured;
    hal_adc_unit_t unit;
    hal_adc_atten_t attenuation;
    hal_adc_width_t width;
    uint8_t samples;
    adc_cali_handle_t cali_handle;
    bool calibrated;
} adc_channel_state_t;

static adc_channel_state_t s_channels[HAL_ADC_MAX_CHANNELS];
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static adc_oneshot_unit_handle_t s_adc2_handle = NULL;
static bool s_adc_initialized = false;

/* ============================================================================
 * Internal Functions
 * ============================================================================ */

static adc_atten_t map_atten(hal_adc_atten_t atten) {
    switch (atten) {
        case HAL_ADC_ATTEN_0DB:   return ADC_ATTEN_DB_0;
        case HAL_ADC_ATTEN_2_5DB: return ADC_ATTEN_DB_2_5;
        case HAL_ADC_ATTEN_6DB:   return ADC_ATTEN_DB_6;
        case HAL_ADC_ATTEN_11DB:  return ADC_ATTEN_DB_12;
        default:                  return ADC_ATTEN_DB_12;
    }
}

static adc_bitwidth_t map_width(hal_adc_width_t width) {
    switch (width) {
        case HAL_ADC_WIDTH_9BIT:  return ADC_BITWIDTH_9;
        case HAL_ADC_WIDTH_10BIT: return ADC_BITWIDTH_10;
        case HAL_ADC_WIDTH_11BIT: return ADC_BITWIDTH_11;
        case HAL_ADC_WIDTH_12BIT: return ADC_BITWIDTH_12;
        case HAL_ADC_WIDTH_13BIT: return ADC_BITWIDTH_13;
        default:                  return ADC_BITWIDTH_12;
    }
}

static adc_oneshot_unit_handle_t* get_unit_handle(hal_adc_unit_t unit) {
    return (unit == HAL_ADC_UNIT_1) ? &s_adc1_handle : &s_adc2_handle;
}

static bool init_calibration(hal_adc_channel_t channel, hal_adc_unit_t unit, hal_adc_atten_t atten) {
    adc_cali_handle_t handle = NULL;
    bool calibrated = false;
    
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = (unit == HAL_ADC_UNIT_1) ? ADC_UNIT_1 : ADC_UNIT_2,
        .atten = map_atten(atten),
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &handle) == ESP_OK) {
        calibrated = true;
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = (unit == HAL_ADC_UNIT_1) ? ADC_UNIT_1 : ADC_UNIT_2,
        .atten = map_atten(atten),
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_config, &handle) == ESP_OK) {
        calibrated = true;
    }
#endif
    
    if (calibrated && channel < HAL_ADC_MAX_CHANNELS) {
        s_channels[channel].cali_handle = handle;
        s_channels[channel].calibrated = true;
    }
    
    return calibrated;
}

/* ============================================================================
 * API Implementation
 * ============================================================================ */

hal_err_t hal_adc_init(void) {
    if (s_adc_initialized) {
        return HAL_OK;
    }
    
    memset(s_channels, 0, sizeof(s_channels));
    s_adc_initialized = true;
    
    ESP_LOGI(TAG, "ADC subsystem initialized");
    return HAL_OK;
}

hal_err_t hal_adc_deinit(void) {
    if (!s_adc_initialized) {
        return HAL_OK;
    }
    
    /* Clean up calibration handles */
    for (int i = 0; i < HAL_ADC_MAX_CHANNELS; i++) {
        if (s_channels[i].calibrated && s_channels[i].cali_handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
            adc_cali_delete_scheme_curve_fitting(s_channels[i].cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
            adc_cali_delete_scheme_line_fitting(s_channels[i].cali_handle);
#endif
        }
    }
    
    /* Delete ADC handles */
    if (s_adc1_handle) {
        adc_oneshot_del_unit(s_adc1_handle);
        s_adc1_handle = NULL;
    }
    if (s_adc2_handle) {
        adc_oneshot_del_unit(s_adc2_handle);
        s_adc2_handle = NULL;
    }
    
    memset(s_channels, 0, sizeof(s_channels));
    s_adc_initialized = false;
    
    ESP_LOGI(TAG, "ADC subsystem deinitialized");
    return HAL_OK;
}

hal_err_t hal_adc_configure(const hal_adc_config_t* config) {
    if (!config || config->channel < 0 || config->channel >= HAL_ADC_MAX_CHANNELS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (!s_adc_initialized) {
        hal_adc_init();
    }
    
    adc_oneshot_unit_handle_t* unit_handle = get_unit_handle(config->unit);
    adc_unit_t adc_unit = (config->unit == HAL_ADC_UNIT_1) ? ADC_UNIT_1 : ADC_UNIT_2;
    
    /* Initialize unit if needed */
    if (*unit_handle == NULL) {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = adc_unit,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        esp_err_t err = adc_oneshot_new_unit(&init_config, unit_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init ADC unit: %s", esp_err_to_name(err));
            return HAL_ERR_FAILED;
        }
    }
    
    /* Configure channel */
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = map_width(config->width),
        .atten = map_atten(config->attenuation),
    };
    
    esp_err_t err = adc_oneshot_config_channel(*unit_handle, (adc_channel_t)config->channel, &chan_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel %d: %s", config->channel, esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }
    
    /* Initialize calibration */
    init_calibration(config->channel, config->unit, config->attenuation);
    
    /* Store configuration */
    s_channels[config->channel].configured = true;
    s_channels[config->channel].unit = config->unit;
    s_channels[config->channel].attenuation = config->attenuation;
    s_channels[config->channel].width = config->width;
    s_channels[config->channel].samples = config->samples;
    
    HAL_LOGD("Configured ADC channel %d: atten=%d, width=%d", 
             config->channel, config->attenuation, config->width);
    
    return HAL_OK;
}

hal_err_t hal_adc_set_channel(hal_adc_channel_t channel, hal_adc_atten_t atten) {
    hal_adc_config_t config = HAL_ADC_CONFIG_DEFAULT();
    config.channel = channel;
    config.attenuation = atten;
    return hal_adc_configure(&config);
}

hal_err_t hal_adc_read_raw(hal_adc_channel_t channel, uint32_t* raw_value) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS || !raw_value) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (!s_channels[channel].configured) {
        return HAL_ERR_NOT_INITIALIZED;
    }
    
    adc_oneshot_unit_handle_t* unit_handle = get_unit_handle(s_channels[channel].unit);
    if (!*unit_handle) {
        return HAL_ERR_NOT_INITIALIZED;
    }
    
    int adc_reading = 0;
    esp_err_t err = adc_oneshot_read(*unit_handle, (adc_channel_t)channel, &adc_reading);
    if (err != ESP_OK) {
        return HAL_ERR_FAILED;
    }
    
    *raw_value = (uint32_t)adc_reading;
    return HAL_OK;
}

hal_err_t hal_adc_read_voltage(hal_adc_channel_t channel, float* voltage_mv) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS || !voltage_mv) {
        return HAL_ERR_INVALID_ARG;
    }
    
    uint32_t raw;
    hal_err_t err = hal_adc_read_raw(channel, &raw);
    if (err != HAL_OK) {
        return err;
    }
    
    /* Use calibrated conversion if available */
    if (s_channels[channel].calibrated && s_channels[channel].cali_handle) {
        int voltage;
        esp_err_t cal_err = adc_cali_raw_to_voltage(s_channels[channel].cali_handle, (int)raw, &voltage);
        if (cal_err == ESP_OK) {
            *voltage_mv = (float)voltage;
            return HAL_OK;
        }
    }
    
    /* Fallback to linear conversion */
    *voltage_mv = hal_adc_raw_to_voltage(channel, raw);
    return HAL_OK;
}

hal_err_t hal_adc_read_average(hal_adc_channel_t channel, uint8_t num_samples, uint32_t* avg_value) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS || !avg_value || num_samples == 0) {
        return HAL_ERR_INVALID_ARG;
    }
    
    uint64_t sum = 0;
    uint32_t raw;
    
    for (uint8_t i = 0; i < num_samples; i++) {
        hal_err_t err = hal_adc_read_raw(channel, &raw);
        if (err != HAL_OK) {
            return err;
        }
        sum += raw;
    }
    
    *avg_value = (uint32_t)(sum / num_samples);
    return HAL_OK;
}

uint32_t hal_adc_get_max_value(hal_adc_channel_t channel) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS) {
        return 4095; /* Default 12-bit */
    }
    
    switch (s_channels[channel].width) {
        case HAL_ADC_WIDTH_9BIT:  return 511;
        case HAL_ADC_WIDTH_10BIT: return 1023;
        case HAL_ADC_WIDTH_11BIT: return 2047;
        case HAL_ADC_WIDTH_12BIT: return 4095;
        case HAL_ADC_WIDTH_13BIT: return 8191;
        default:                  return 4095;
    }
}

float hal_adc_raw_to_voltage(hal_adc_channel_t channel, uint32_t raw_value) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS) {
        return 0.0f;
    }
    
    float max_voltage;
    switch (s_channels[channel].attenuation) {
        case HAL_ADC_ATTEN_0DB:   max_voltage = 1100.0f; break;
        case HAL_ADC_ATTEN_2_5DB: max_voltage = 1500.0f; break;
        case HAL_ADC_ATTEN_6DB:   max_voltage = 2200.0f; break;
        case HAL_ADC_ATTEN_11DB:  max_voltage = 3900.0f; break;
        default:                  max_voltage = 3900.0f; break;
    }
    
    uint32_t max_raw = hal_adc_get_max_value(channel);
    return (raw_value * max_voltage) / max_raw;
}

hal_err_t hal_adc_reset(hal_adc_channel_t channel) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    /* Clean up calibration */
    if (s_channels[channel].calibrated && s_channels[channel].cali_handle) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_delete_scheme_curve_fitting(s_channels[channel].cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(s_channels[channel].cali_handle);
#endif
    }
    
    memset(&s_channels[channel], 0, sizeof(adc_channel_state_t));
    return HAL_OK;
}

/* ESP32-specific ADC to GPIO mapping */
hal_gpio_pin_t hal_adc_channel_to_gpio(hal_adc_channel_t channel) {
    /* ADC1 channels */
    static const int8_t adc1_gpio[] = {36, 37, 38, 39, 32, 33, 34, 35};
    
    if (channel >= 0 && channel < 8) {
        return (hal_gpio_pin_t)adc1_gpio[channel];
    }
    return HAL_GPIO_PIN_INVALID;
}

hal_adc_channel_t hal_adc_gpio_to_channel(hal_gpio_pin_t pin) {
    /* Reverse mapping */
    switch (pin) {
        case 36: return 0;
        case 37: return 1;
        case 38: return 2;
        case 39: return 3;
        case 32: return 4;
        case 33: return 5;
        case 34: return 6;
        case 35: return 7;
        default: return HAL_ADC_CHANNEL_INVALID;
    }
}

#endif /* HAL_PLATFORM_ESP_IDF */