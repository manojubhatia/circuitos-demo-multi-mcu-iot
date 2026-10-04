/**
 * @file hal_types.h
 * @brief Hardware Abstraction Layer - Common Types

 * 
 * Provides portable type definitions for HAL layer.
 * These types abstract hardware-specific details and enable
 * code portability across different platforms.
 */

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Codes
 * ============================================================================ */

typedef enum {
    HAL_OK = 0,
    HAL_ERR_INVALID_ARG,
    HAL_ERR_NOT_INITIALIZED,
    HAL_ERR_ALREADY_INITIALIZED,
    HAL_ERR_TIMEOUT,
    HAL_ERR_BUSY,
    HAL_ERR_NO_MEMORY,
    HAL_ERR_NOT_SUPPORTED,
    HAL_ERR_NOT_FOUND,
    HAL_ERR_INVALID_STATE,
    HAL_ERR_FAILED,
} hal_err_t;

/* ============================================================================
 * Hardware Resource Types
 * ============================================================================ */

typedef int32_t hal_gpio_pin_t;
typedef int32_t hal_adc_channel_t;
typedef int32_t hal_pwm_channel_t;
typedef int32_t hal_uart_port_t;
typedef int32_t hal_spi_host_t;
typedef int32_t hal_i2c_port_t;
typedef int32_t hal_timer_id_t;

/* Invalid resource markers */
#define HAL_GPIO_PIN_INVALID    (-1)
#define HAL_ADC_CHANNEL_INVALID (-1)
#define HAL_PWM_CHANNEL_INVALID (-1)

/* ============================================================================
 * Time Types
 * ============================================================================ */

typedef uint32_t hal_ms_t;
typedef uint64_t hal_us_t;

#define HAL_WAIT_FOREVER    ((hal_ms_t)0xFFFFFFFF)
#define HAL_NO_WAIT         ((hal_ms_t)0)

/* ============================================================================
 * Callback Types
 * ============================================================================ */

typedef void (*hal_isr_callback_t)(void* arg);
typedef void (*hal_event_callback_t)(void* arg, uint32_t event);

/* ============================================================================
 * String Helper
 * ============================================================================ */

static inline const char* hal_err_to_str(hal_err_t err) {
    switch (err) {
        case HAL_OK:                   return "OK";
        case HAL_ERR_INVALID_ARG:      return "Invalid argument";
        case HAL_ERR_NOT_INITIALIZED:  return "Not initialized";
        case HAL_ERR_ALREADY_INITIALIZED: return "Already initialized";
        case HAL_ERR_TIMEOUT:          return "Timeout";
        case HAL_ERR_BUSY:             return "Busy";
        case HAL_ERR_NO_MEMORY:        return "No memory";
        case HAL_ERR_NOT_SUPPORTED:    return "Not supported";
        case HAL_ERR_NOT_FOUND:        return "Not found";
        case HAL_ERR_INVALID_STATE:    return "Invalid state";
        case HAL_ERR_FAILED:           return "Failed";
        default:                       return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* HAL_TYPES_H */