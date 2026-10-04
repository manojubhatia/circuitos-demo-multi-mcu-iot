/**
 * @file hal_adc_mock.c
 * @brief Hardware Abstraction Layer - ADC Mock Implementation

 * 
 * Mock implementation for unit testing.
 */

#include "hal_adc.h"

#if HAL_PLATFORM_MOCK

#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Mock State
 * ============================================================================ */

typedef struct {
    bool configured;
    hal_adc_unit_t unit;
    hal_adc_atten_t attenuation;
    hal_adc_width_t width;
    uint32_t raw_value;     /* Simulated raw value */
    float voltage_mv;       /* Simulated voltage */
    bool use_voltage;       /* If true, return voltage_mv, else compute from raw */
} mock_adc_state_t;

static mock_adc_state_t s_channels[HAL_ADC_MAX_CHANNELS];
static bool s_adc_initialized = false;

/* Error injection */
static hal_adc_channel_t s_fail_channel = HAL_ADC_CHANNEL_INVALID;
static hal_err_t s_fail_error = HAL_OK;

/* ============================================================================
 * Mock Control API (for test setup)
 * ============================================================================ */

void hal_adc_mock_reset(void) {
    memset(s_channels, 0, sizeof(s_channels));
    s_adc_initialized = false;
    s_fail_channel = HAL_ADC_CHANNEL_INVALID;
    s_fail_error = HAL_OK;
}

void hal_adc_mock_set_raw(hal_adc_channel_t channel, uint32_t value) {
    if (channel >= 0 && channel < HAL_ADC_MAX_CHANNELS) {
        s_channels[channel].raw_value = value;
        s_channels[channel].use_voltage = false;
    }
}

void hal_adc_mock_set_voltage(hal_adc_channel_t channel, float voltage_mv) {
    if (channel >= 0 && channel < HAL_ADC_MAX_CHANNELS) {
        s_channels[channel].voltage_mv = voltage_mv;
        s_channels[channel].use_voltage = true;
    }
}

uint32_t hal_adc_mock_get_raw(hal_adc_channel_t channel) {
    if (channel >= 0 && channel < HAL_ADC_MAX_CHANNELS) {
        return s_channels[channel].raw_value;
    }
    return 0;
}

void hal_adc_mock_inject_error(hal_adc_channel_t channel, hal_err_t error) {
    s_fail_channel = channel;
    s_fail_error = error;
}

bool hal_adc_mock_is_configured(hal_adc_channel_t channel) {
    if (channel >= 0 && channel < HAL_ADC_MAX_CHANNELS) {
        return s_channels[channel].configured;
    }
    return false;
}

/* ============================================================================
 * HAL API Implementation (Mock)
 * ============================================================================ */

hal_err_t hal_adc_init(void) {
    if (s_adc_initialized) {
        return HAL_OK;
    }
    memset(s_channels, 0, sizeof(s_channels));
    s_adc_initialized = true;
    return HAL_OK;
}

hal_err_t hal_adc_deinit(void) {
    memset(s_channels, 0, sizeof(s_channels));
    s_adc_initialized = false;
    return HAL_OK;
}

hal_err_t hal_adc_configure(const hal_adc_config_t* config) {
    if (!config || config->channel < 0 || config->channel >= HAL_ADC_MAX_CHANNELS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (config->channel == s_fail_channel && s_fail_error != HAL_OK) {
        return s_fail_error;
    }
    
    if (!s_adc_initialized) {
        hal_adc_init();
    }
    
    s_channels[config->channel].configured = true;
    s_channels[config->channel].unit = config->unit;
    s_channels[config->channel].attenuation = config->attenuation;
    s_channels[config->channel].width = config->width;
    
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
    
    if (channel == s_fail_channel && s_fail_error != HAL_OK) {
        return s_fail_error;
    }
    
    if (!s_channels[channel].configured) {
        return HAL_ERR_NOT_INITIALIZED;
    }
    
    *raw_value = s_channels[channel].raw_value;
    return HAL_OK;
}

hal_err_t hal_adc_read_voltage(hal_adc_channel_t channel, float* voltage_mv) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS || !voltage_mv) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (channel == s_fail_channel && s_fail_error != HAL_OK) {
        return s_fail_error;
    }
    
    if (!s_channels[channel].configured) {
        return HAL_ERR_NOT_INITIALIZED;
    }
    
    if (s_channels[channel].use_voltage) {
        *voltage_mv = s_channels[channel].voltage_mv;
    } else {
        /* Convert raw to voltage */
        *voltage_mv = hal_adc_raw_to_voltage(channel, s_channels[channel].raw_value);
    }
    
    return HAL_OK;
}

hal_err_t hal_adc_read_average(hal_adc_channel_t channel, uint8_t num_samples, uint32_t* avg_value) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS || !avg_value || num_samples == 0) {
        return HAL_ERR_INVALID_ARG;
    }
    
    /* Mock just returns the single value */
    return hal_adc_read_raw(channel, avg_value);
}

uint32_t hal_adc_get_max_value(hal_adc_channel_t channel) {
    if (channel < 0 || channel >= HAL_ADC_MAX_CHANNELS) {
        return 4095;
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
    
    memset(&s_channels[channel], 0, sizeof(mock_adc_state_t));
    return HAL_OK;
}

hal_gpio_pin_t hal_adc_channel_to_gpio(hal_adc_channel_t channel) {
    /* Mock: return pin = channel + 32 */
    if (channel >= 0 && channel < HAL_ADC_MAX_CHANNELS) {
        return (hal_gpio_pin_t)(channel + 32);
    }
    return HAL_GPIO_PIN_INVALID;
}

hal_adc_channel_t hal_adc_gpio_to_channel(hal_gpio_pin_t pin) {
    /* Mock: channel = pin - 32 */
    if (pin >= 32 && pin < 32 + HAL_ADC_MAX_CHANNELS) {
        return (hal_adc_channel_t)(pin - 32);
    }
    return HAL_ADC_CHANNEL_INVALID;
}

#endif /* HAL_PLATFORM_MOCK */