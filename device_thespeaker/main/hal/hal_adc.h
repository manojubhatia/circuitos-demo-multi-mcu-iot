/**
 * @file hal_adc.h
 * @brief Hardware Abstraction Layer - ADC Interface

 * 
 * Provides a portable ADC API that abstracts hardware-specific
 * analog-to-digital converter implementations.
 */

#ifndef HAL_ADC_H
#define HAL_ADC_H

#include "hal_types.h"
#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ADC Configuration
 * ============================================================================ */

typedef enum {
    HAL_ADC_ATTEN_0DB,      /* Full-scale ~1.1V (ESP32) */
    HAL_ADC_ATTEN_2_5DB,    /* Full-scale ~1.5V */
    HAL_ADC_ATTEN_6DB,      /* Full-scale ~2.2V */
    HAL_ADC_ATTEN_11DB,     /* Full-scale ~3.9V (default) */
} hal_adc_atten_t;

typedef enum {
    HAL_ADC_WIDTH_9BIT  = 9,
    HAL_ADC_WIDTH_10BIT = 10,
    HAL_ADC_WIDTH_11BIT = 11,
    HAL_ADC_WIDTH_12BIT = 12,
    HAL_ADC_WIDTH_13BIT = 13,  /* Some chips */
} hal_adc_width_t;

typedef enum {
    HAL_ADC_UNIT_1 = 0,
    HAL_ADC_UNIT_2 = 1,
} hal_adc_unit_t;

typedef struct {
    hal_adc_channel_t channel;
    hal_adc_unit_t unit;
    hal_adc_atten_t attenuation;
    hal_adc_width_t width;
    uint8_t samples;            /* Number of samples to average */
} hal_adc_config_t;

/* Default configuration */
#define HAL_ADC_CONFIG_DEFAULT() { \
    .channel = HAL_ADC_CHANNEL_INVALID, \
    .unit = HAL_ADC_UNIT_1,             \
    .attenuation = HAL_ADC_ATTEN_11DB,  \
    .width = HAL_ADC_WIDTH_12BIT,       \
    .samples = HAL_ADC_DEFAULT_SAMPLES, \
}

/* ============================================================================
 * ADC API Functions
 * ============================================================================ */

/**
 * @brief Initialize ADC subsystem
 * @return HAL_OK on success
 */
hal_err_t hal_adc_init(void);

/**
 * @brief Deinitialize ADC subsystem
 * @return HAL_OK on success
 */
hal_err_t hal_adc_deinit(void);

/**
 * @brief Configure an ADC channel
 * @param config Pointer to configuration structure
 * @return HAL_OK on success
 */
hal_err_t hal_adc_configure(const hal_adc_config_t* config);

/**
 * @brief Configure ADC channel with simple parameters
 * @param channel ADC channel number
 * @param atten Attenuation setting
 * @return HAL_OK on success
 */
hal_err_t hal_adc_set_channel(hal_adc_channel_t channel, hal_adc_atten_t atten);

/**
 * @brief Read raw ADC value
 * @param channel ADC channel number
 * @param raw_value Pointer to store raw value
 * @return HAL_OK on success
 */
hal_err_t hal_adc_read_raw(hal_adc_channel_t channel, uint32_t* raw_value);

/**
 * @brief Read ADC value as voltage in millivolts
 * @param channel ADC channel number
 * @param voltage_mv Pointer to store voltage
 * @return HAL_OK on success
 */
hal_err_t hal_adc_read_voltage(hal_adc_channel_t channel, float* voltage_mv);

/**
 * @brief Read multiple samples and return average
 * @param channel ADC channel number
 * @param num_samples Number of samples to average
 * @param avg_value Pointer to store average
 * @return HAL_OK on success
 */
hal_err_t hal_adc_read_average(hal_adc_channel_t channel, uint8_t num_samples, uint32_t* avg_value);

/**
 * @brief Get maximum raw value for current resolution
 * @param channel ADC channel number
 * @return Maximum raw value (e.g., 4095 for 12-bit)
 */
uint32_t hal_adc_get_max_value(hal_adc_channel_t channel);

/**
 * @brief Convert raw value to voltage
 * @param channel ADC channel number
 * @param raw_value Raw ADC value
 * @return Voltage in millivolts
 */
float hal_adc_raw_to_voltage(hal_adc_channel_t channel, uint32_t raw_value);

/**
 * @brief Reset ADC channel configuration
 * @param channel ADC channel number
 * @return HAL_OK on success
 */
hal_err_t hal_adc_reset(hal_adc_channel_t channel);

/* ============================================================================
 * Platform-specific helpers
 * ============================================================================ */

/**
 * @brief Get GPIO pin number for ADC channel (platform-specific)
 * @param channel ADC channel
 * @return GPIO pin number or -1 if not applicable
 */
hal_gpio_pin_t hal_adc_channel_to_gpio(hal_adc_channel_t channel);

/**
 * @brief Get ADC channel for GPIO pin (platform-specific)
 * @param pin GPIO pin number
 * @return ADC channel or HAL_ADC_CHANNEL_INVALID
 */
hal_adc_channel_t hal_adc_gpio_to_channel(hal_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* HAL_ADC_H */