/**
 * @file hal_gpio_mock.c
 * @brief Hardware Abstraction Layer - GPIO Mock Implementation

 * 
 * Mock implementation for unit testing. Allows setting expectations
 * and verifying GPIO operations without actual hardware.
 */

#include "hal_gpio.h"

#if HAL_PLATFORM_MOCK

#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Mock State
 * ============================================================================ */

typedef struct {
    bool configured;
    hal_gpio_mode_t mode;
    hal_gpio_pull_t pull;
    hal_gpio_drive_t drive;
    bool level;
    hal_gpio_intr_type_t intr_type;
    hal_isr_callback_t isr_handler;
    void* isr_arg;
    bool intr_enabled;
#if HAL_ENABLE_STATS
    hal_gpio_stats_t stats;
#endif
} mock_gpio_state_t;

typedef struct {
    hal_gpio_pin_t pin;
    hal_gpio_mode_t mode;
    hal_gpio_pull_t pull;
    bool called;
} mock_expectation_t;

static mock_gpio_state_t s_pin_states[HAL_GPIO_MAX_PINS];
static mock_expectation_t s_expectations[HAL_GPIO_MAX_PINS];
static int s_expectation_count = 0;
static bool s_gpio_initialized = false;

/* Error injection */
static hal_gpio_pin_t s_fail_pin = HAL_GPIO_PIN_INVALID;
static hal_err_t s_fail_error = HAL_OK;

/* ============================================================================
 * Mock Control API (for test setup)
 * ============================================================================ */

void hal_gpio_mock_reset(void) {
    memset(s_pin_states, 0, sizeof(s_pin_states));
    memset(s_expectations, 0, sizeof(s_expectations));
    s_expectation_count = 0;
    s_gpio_initialized = false;
    s_fail_pin = HAL_GPIO_PIN_INVALID;
    s_fail_error = HAL_OK;
}

void hal_gpio_mock_set_level(hal_gpio_pin_t pin, bool level) {
    if (pin >= 0 && pin < HAL_GPIO_MAX_PINS) {
        s_pin_states[pin].level = level;
    }
}

bool hal_gpio_mock_get_level(hal_gpio_pin_t pin) {
    if (pin >= 0 && pin < HAL_GPIO_MAX_PINS) {
        return s_pin_states[pin].level;
    }
    return false;
}

void hal_gpio_mock_expect_init(hal_gpio_pin_t pin, hal_gpio_mode_t mode, hal_gpio_pull_t pull) {
    if (s_expectation_count < HAL_GPIO_MAX_PINS) {
        s_expectations[s_expectation_count].pin = pin;
        s_expectations[s_expectation_count].mode = mode;
        s_expectations[s_expectation_count].pull = pull;
        s_expectations[s_expectation_count].called = false;
        s_expectation_count++;
    }
}

bool hal_gpio_mock_verify(void) {
    bool all_met = true;
    for (int i = 0; i < s_expectation_count; i++) {
        if (!s_expectations[i].called) {
            printf("Mock: Expected call to configure pin %ld not made\n", 
                   (long)s_expectations[i].pin);
            all_met = false;
        }
    }
    return all_met;
}

void hal_gpio_mock_inject_error(hal_gpio_pin_t pin, hal_err_t error) {
    s_fail_pin = pin;
    s_fail_error = error;
}

void hal_gpio_mock_trigger_interrupt(hal_gpio_pin_t pin) {
    if (pin >= 0 && pin < HAL_GPIO_MAX_PINS) {
        if (s_pin_states[pin].intr_enabled && s_pin_states[pin].isr_handler) {
#if HAL_ENABLE_STATS
            s_pin_states[pin].stats.isr_count++;
#endif
            s_pin_states[pin].isr_handler(s_pin_states[pin].isr_arg);
        }
    }
}

bool hal_gpio_mock_is_configured(hal_gpio_pin_t pin) {
    if (pin >= 0 && pin < HAL_GPIO_MAX_PINS) {
        return s_pin_states[pin].configured;
    }
    return false;
}

hal_gpio_mode_t hal_gpio_mock_get_mode(hal_gpio_pin_t pin) {
    if (pin >= 0 && pin < HAL_GPIO_MAX_PINS) {
        return s_pin_states[pin].mode;
    }
    return HAL_GPIO_MODE_DISABLED;
}

/* ============================================================================
 * HAL API Implementation (Mock)
 * ============================================================================ */

hal_err_t hal_gpio_init(void) {
    if (s_gpio_initialized) {
        return HAL_OK;
    }
    memset(s_pin_states, 0, sizeof(s_pin_states));
    s_gpio_initialized = true;
    return HAL_OK;
}

hal_err_t hal_gpio_deinit(void) {
    memset(s_pin_states, 0, sizeof(s_pin_states));
    s_gpio_initialized = false;
    return HAL_OK;
}

hal_err_t hal_gpio_configure(const hal_gpio_config_t* config) {
    if (!config || config->pin < 0 || config->pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    /* Check for error injection */
    if (config->pin == s_fail_pin && s_fail_error != HAL_OK) {
        return s_fail_error;
    }
    
    if (!s_gpio_initialized) {
        hal_gpio_init();
    }
    
    /* Store configuration */
    s_pin_states[config->pin].configured = true;
    s_pin_states[config->pin].mode = config->mode;
    s_pin_states[config->pin].pull = config->pull;
    s_pin_states[config->pin].drive = config->drive;
    s_pin_states[config->pin].intr_type = config->intr_type;
    s_pin_states[config->pin].isr_handler = config->isr_handler;
    s_pin_states[config->pin].isr_arg = config->isr_arg;
    
    /* Mark expectations as met */
    for (int i = 0; i < s_expectation_count; i++) {
        if (s_expectations[i].pin == config->pin &&
            s_expectations[i].mode == config->mode &&
            s_expectations[i].pull == config->pull) {
            s_expectations[i].called = true;
        }
    }
    
    return HAL_OK;
}

hal_err_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode, hal_gpio_pull_t pull) {
    hal_gpio_config_t config = HAL_GPIO_CONFIG_DEFAULT();
    config.pin = pin;
    config.mode = mode;
    config.pull = pull;
    return hal_gpio_configure(&config);
}

hal_err_t hal_gpio_set_level(hal_gpio_pin_t pin, bool level) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (pin == s_fail_pin && s_fail_error != HAL_OK) {
        return s_fail_error;
    }
    
    s_pin_states[pin].level = level;
    
#if HAL_ENABLE_STATS
    s_pin_states[pin].stats.set_count++;
#endif
    
    return HAL_OK;
}

bool hal_gpio_get_level(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return false;
    }
    
#if HAL_ENABLE_STATS
    s_pin_states[pin].stats.get_count++;
#endif
    
    return s_pin_states[pin].level;
}

hal_err_t hal_gpio_toggle(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (pin == s_fail_pin && s_fail_error != HAL_OK) {
        return s_fail_error;
    }
    
    s_pin_states[pin].level = !s_pin_states[pin].level;
    
#if HAL_ENABLE_STATS
    s_pin_states[pin].stats.toggle_count++;
#endif
    
    return HAL_OK;
}

hal_err_t hal_gpio_set_drive(hal_gpio_pin_t pin, hal_gpio_drive_t drive) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    s_pin_states[pin].drive = drive;
    return HAL_OK;
}

hal_err_t hal_gpio_set_interrupt(hal_gpio_pin_t pin, hal_gpio_intr_type_t type,
                                  hal_isr_callback_t handler, void* arg) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    s_pin_states[pin].intr_type = type;
    s_pin_states[pin].isr_handler = handler;
    s_pin_states[pin].isr_arg = arg;
    s_pin_states[pin].intr_enabled = (type != HAL_GPIO_INTR_DISABLE && handler != NULL);
    
    return HAL_OK;
}

hal_err_t hal_gpio_intr_enable(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    s_pin_states[pin].intr_enabled = true;
    return HAL_OK;
}

hal_err_t hal_gpio_intr_disable(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    s_pin_states[pin].intr_enabled = false;
    return HAL_OK;
}

hal_err_t hal_gpio_reset(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    memset(&s_pin_states[pin], 0, sizeof(mock_gpio_state_t));
    return HAL_OK;
}

#if HAL_ENABLE_STATS
hal_err_t hal_gpio_get_stats(hal_gpio_pin_t pin, hal_gpio_stats_t* stats) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS || !stats) {
        return HAL_ERR_INVALID_ARG;
    }
    
    *stats = s_pin_states[pin].stats;
    return HAL_OK;
}

void hal_gpio_reset_stats(hal_gpio_pin_t pin) {
    if (pin >= 0 && pin < HAL_GPIO_MAX_PINS) {
        memset(&s_pin_states[pin].stats, 0, sizeof(hal_gpio_stats_t));
    }
}
#endif

#endif /* HAL_PLATFORM_MOCK */