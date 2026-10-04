/**
 * @file hal_gpio.h
 * @brief Hardware Abstraction Layer - GPIO Interface

 * 
 * Provides a portable GPIO API that abstracts hardware-specific
 * GPIO implementations. Supports ESP-IDF, Arduino, Zephyr, and mock.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "hal_types.h"
#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GPIO Mode Configuration
 * ============================================================================ */

typedef enum {
    HAL_GPIO_MODE_DISABLED,
    HAL_GPIO_MODE_INPUT,
    HAL_GPIO_MODE_OUTPUT,
    HAL_GPIO_MODE_OUTPUT_OD,        /* Open-drain output */
    HAL_GPIO_MODE_INPUT_OUTPUT,     /* Bidirectional */
    HAL_GPIO_MODE_INPUT_OUTPUT_OD,  /* Bidirectional open-drain */
} hal_gpio_mode_t;

typedef enum {
    HAL_GPIO_PULL_NONE,
    HAL_GPIO_PULL_UP,
    HAL_GPIO_PULL_DOWN,
    HAL_GPIO_PULL_UP_DOWN,          /* Both (for some chips) */
} hal_gpio_pull_t;

typedef enum {
    HAL_GPIO_DRIVE_WEAK,
    HAL_GPIO_DRIVE_MEDIUM,
    HAL_GPIO_DRIVE_STRONG,
    HAL_GPIO_DRIVE_STRONGEST,
} hal_gpio_drive_t;

/* ============================================================================
 * GPIO Interrupt Configuration
 * ============================================================================ */

typedef enum {
    HAL_GPIO_INTR_DISABLE,
    HAL_GPIO_INTR_RISING,
    HAL_GPIO_INTR_FALLING,
    HAL_GPIO_INTR_BOTH_EDGE,
    HAL_GPIO_INTR_LOW_LEVEL,
    HAL_GPIO_INTR_HIGH_LEVEL,
} hal_gpio_intr_type_t;

/* ============================================================================
 * GPIO Configuration Structure
 * ============================================================================ */

typedef struct {
    hal_gpio_pin_t pin;
    hal_gpio_mode_t mode;
    hal_gpio_pull_t pull;
    hal_gpio_drive_t drive;
    hal_gpio_intr_type_t intr_type;
    hal_isr_callback_t isr_handler;
    void* isr_arg;
} hal_gpio_config_t;

/* Default configuration macro */
#define HAL_GPIO_CONFIG_DEFAULT() { \
    .pin = HAL_GPIO_PIN_INVALID,    \
    .mode = HAL_GPIO_MODE_INPUT,    \
    .pull = HAL_GPIO_PULL_NONE,     \
    .drive = HAL_GPIO_DRIVE_MEDIUM, \
    .intr_type = HAL_GPIO_INTR_DISABLE, \
    .isr_handler = NULL,            \
    .isr_arg = NULL,                \
}

/* ============================================================================
 * GPIO API Functions
 * ============================================================================ */

/**
 * @brief Initialize GPIO subsystem
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_init(void);

/**
 * @brief Deinitialize GPIO subsystem
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_deinit(void);

/**
 * @brief Configure a GPIO pin
 * @param config Pointer to configuration structure
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_configure(const hal_gpio_config_t* config);

/**
 * @brief Configure a GPIO pin with simple parameters
 * @param pin GPIO pin number
 * @param mode GPIO mode
 * @param pull Pull-up/down configuration
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode, hal_gpio_pull_t pull);

/**
 * @brief Set GPIO output level
 * @param pin GPIO pin number
 * @param level Output level (true = high, false = low)
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_set_level(hal_gpio_pin_t pin, bool level);

/**
 * @brief Get GPIO input level
 * @param pin GPIO pin number
 * @return Current level (true = high, false = low)
 */
bool hal_gpio_get_level(hal_gpio_pin_t pin);

/**
 * @brief Toggle GPIO output level
 * @param pin GPIO pin number
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_toggle(hal_gpio_pin_t pin);

/**
 * @brief Set drive strength
 * @param pin GPIO pin number
 * @param drive Drive strength
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_set_drive(hal_gpio_pin_t pin, hal_gpio_drive_t drive);

/**
 * @brief Configure GPIO interrupt
 * @param pin GPIO pin number
 * @param type Interrupt type
 * @param handler ISR callback function
 * @param arg Argument passed to callback
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_set_interrupt(hal_gpio_pin_t pin, hal_gpio_intr_type_t type, 
                                  hal_isr_callback_t handler, void* arg);

/**
 * @brief Enable GPIO interrupt
 * @param pin GPIO pin number
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_intr_enable(hal_gpio_pin_t pin);

/**
 * @brief Disable GPIO interrupt
 * @param pin GPIO pin number
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_intr_disable(hal_gpio_pin_t pin);

/**
 * @brief Reset pin to default state
 * @param pin GPIO pin number
 * @return HAL_OK on success
 */
hal_err_t hal_gpio_reset(hal_gpio_pin_t pin);

/* ============================================================================
 * Statistics (if enabled)
 * ============================================================================ */

#if HAL_ENABLE_STATS
typedef struct {
    uint32_t set_count;
    uint32_t get_count;
    uint32_t toggle_count;
    uint32_t isr_count;
} hal_gpio_stats_t;

hal_err_t hal_gpio_get_stats(hal_gpio_pin_t pin, hal_gpio_stats_t* stats);
void hal_gpio_reset_stats(hal_gpio_pin_t pin);
#endif

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H */