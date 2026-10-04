/**
 * @file hal_config.h
 * @brief Hardware Abstraction Layer - Configuration

 * 
 * Configures which HAL backend to use based on the target platform.
 */

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

/* ============================================================================
 * Platform Selection
 * Set by build system or template generation
 * ============================================================================ */

#define HAL_PLATFORM_ESP_IDF    1
#define HAL_PLATFORM_ARDUINO    0
#define HAL_PLATFORM_ZEPHYR     0
#define HAL_PLATFORM_LINUX      0
#define HAL_PLATFORM_MOCK       0

/* Override for unit testing - define HAL_USE_MOCK=1 to use mocks */
#ifdef HAL_USE_MOCK
#undef HAL_PLATFORM_ESP_IDF
#undef HAL_PLATFORM_ARDUINO
#undef HAL_PLATFORM_ZEPHYR
#undef HAL_PLATFORM_LINUX
#undef HAL_PLATFORM_MOCK
#define HAL_PLATFORM_ESP_IDF    0
#define HAL_PLATFORM_ARDUINO    0
#define HAL_PLATFORM_ZEPHYR     0
#define HAL_PLATFORM_LINUX      0
#define HAL_PLATFORM_MOCK       1
#endif

/* ============================================================================
 * Feature Configuration
 * ============================================================================ */

/* GPIO Configuration */
#define HAL_GPIO_MAX_PINS           48
#define HAL_GPIO_MAX_ISR            16

/* ADC Configuration */
#define HAL_ADC_MAX_CHANNELS        10
#define HAL_ADC_DEFAULT_SAMPLES     64

/* PWM Configuration */
#define HAL_PWM_MAX_CHANNELS        16
#define HAL_PWM_DEFAULT_FREQ_HZ     5000
#define HAL_PWM_DEFAULT_RESOLUTION  8

/* UART Configuration */
#define HAL_UART_MAX_PORTS          3
#define HAL_UART_DEFAULT_BAUD       115200
#define HAL_UART_RX_BUFFER_SIZE     256
#define HAL_UART_TX_BUFFER_SIZE     256

/* SPI Configuration */
#define HAL_SPI_MAX_HOSTS           2
#define HAL_SPI_DEFAULT_CLOCK_HZ    1000000

/* I2C Configuration */
#define HAL_I2C_MAX_PORTS           2
#define HAL_I2C_DEFAULT_CLOCK_HZ    100000

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

#ifndef HAL_ENABLE_DEBUG
#define HAL_ENABLE_DEBUG            0
#endif

#ifndef HAL_ENABLE_STATS
#define HAL_ENABLE_STATS            0
#endif

#if HAL_ENABLE_DEBUG
#include "osal/osal_log.h"
#define HAL_LOGD(fmt, ...) osal_log_write(OSAL_LOG_DEBUG, "HAL", fmt, ##__VA_ARGS__)
#define HAL_LOGI(fmt, ...) osal_log_write(OSAL_LOG_INFO,  "HAL", fmt, ##__VA_ARGS__)
#define HAL_LOGW(fmt, ...) osal_log_write(OSAL_LOG_WARN,  "HAL", fmt, ##__VA_ARGS__)
#define HAL_LOGE(fmt, ...) osal_log_write(OSAL_LOG_ERROR, "HAL", fmt, ##__VA_ARGS__)
#else
#define HAL_LOGD(fmt, ...)
#define HAL_LOGI(fmt, ...)
#define HAL_LOGW(fmt, ...)
#define HAL_LOGE(fmt, ...)
#endif

#endif /* HAL_CONFIG_H */