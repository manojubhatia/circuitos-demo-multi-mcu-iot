/**
 * @file hal_gpio_esp.c
 * @brief Hardware Abstraction Layer - GPIO ESP-IDF Implementation

 */

#include "hal_gpio.h"

#if HAL_PLATFORM_ESP_IDF

#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "hal_gpio";

/* ============================================================================
 * Internal State
 * ============================================================================ */

typedef struct {
    bool configured;
    hal_gpio_mode_t mode;
    hal_isr_callback_t isr_handler;
    void* isr_arg;
#if HAL_ENABLE_STATS
    hal_gpio_stats_t stats;
#endif
} gpio_pin_state_t;

static gpio_pin_state_t s_pin_states[HAL_GPIO_MAX_PINS];
static bool s_gpio_initialized = false;
static bool s_isr_service_installed = false;

/* ============================================================================
 * Internal Functions
 * ============================================================================ */

static gpio_mode_t map_mode(hal_gpio_mode_t mode) {
    switch (mode) {
        case HAL_GPIO_MODE_DISABLED:       return GPIO_MODE_DISABLE;
        case HAL_GPIO_MODE_INPUT:          return GPIO_MODE_INPUT;
        case HAL_GPIO_MODE_OUTPUT:         return GPIO_MODE_OUTPUT;
        case HAL_GPIO_MODE_OUTPUT_OD:      return GPIO_MODE_OUTPUT_OD;
        case HAL_GPIO_MODE_INPUT_OUTPUT:   return GPIO_MODE_INPUT_OUTPUT;
        case HAL_GPIO_MODE_INPUT_OUTPUT_OD: return GPIO_MODE_INPUT_OUTPUT_OD;
        default:                           return GPIO_MODE_DISABLE;
    }
}

static __attribute__((unused)) gpio_pull_mode_t map_pull(hal_gpio_pull_t pull) {
    switch (pull) {
        case HAL_GPIO_PULL_UP:      return GPIO_PULLUP_ONLY;
        case HAL_GPIO_PULL_DOWN:    return GPIO_PULLDOWN_ONLY;
        case HAL_GPIO_PULL_UP_DOWN: return GPIO_PULLUP_PULLDOWN;
        default:                    return GPIO_FLOATING;
    }
}

static gpio_int_type_t map_intr_type(hal_gpio_intr_type_t type) {
    switch (type) {
        case HAL_GPIO_INTR_RISING:     return GPIO_INTR_POSEDGE;
        case HAL_GPIO_INTR_FALLING:    return GPIO_INTR_NEGEDGE;
        case HAL_GPIO_INTR_BOTH_EDGE:  return GPIO_INTR_ANYEDGE;
        case HAL_GPIO_INTR_LOW_LEVEL:  return GPIO_INTR_LOW_LEVEL;
        case HAL_GPIO_INTR_HIGH_LEVEL: return GPIO_INTR_HIGH_LEVEL;
        default:                       return GPIO_INTR_DISABLE;
    }
}

static gpio_drive_cap_t map_drive(hal_gpio_drive_t drive) {
    switch (drive) {
        case HAL_GPIO_DRIVE_WEAK:      return GPIO_DRIVE_CAP_0;
        case HAL_GPIO_DRIVE_MEDIUM:    return GPIO_DRIVE_CAP_1;
        case HAL_GPIO_DRIVE_STRONG:    return GPIO_DRIVE_CAP_2;
        case HAL_GPIO_DRIVE_STRONGEST: return GPIO_DRIVE_CAP_3;
        default:                       return GPIO_DRIVE_CAP_2;
    }
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t pin = (uint32_t)(uintptr_t)arg;
    if (pin < HAL_GPIO_MAX_PINS && s_pin_states[pin].isr_handler) {
#if HAL_ENABLE_STATS
        s_pin_states[pin].stats.isr_count++;
#endif
        s_pin_states[pin].isr_handler(s_pin_states[pin].isr_arg);
    }
}

/* ============================================================================
 * API Implementation
 * ============================================================================ */

hal_err_t hal_gpio_init(void) {
    if (s_gpio_initialized) {
        return HAL_OK;
    }
    
    memset(s_pin_states, 0, sizeof(s_pin_states));
    s_gpio_initialized = true;
    
    ESP_LOGI(TAG, "GPIO subsystem initialized");
    return HAL_OK;
}

hal_err_t hal_gpio_deinit(void) {
    if (!s_gpio_initialized) {
        return HAL_OK;
    }
    
    /* Reset all configured pins */
    for (int i = 0; i < HAL_GPIO_MAX_PINS; i++) {
        if (s_pin_states[i].configured) {
            hal_gpio_reset((hal_gpio_pin_t)i);
        }
    }
    
    if (s_isr_service_installed) {
        gpio_uninstall_isr_service();
        s_isr_service_installed = false;
    }
    
    s_gpio_initialized = false;
    ESP_LOGI(TAG, "GPIO subsystem deinitialized");
    return HAL_OK;
}

hal_err_t hal_gpio_configure(const hal_gpio_config_t* config) {
    if (!config || config->pin < 0 || config->pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    if (!s_gpio_initialized) {
        hal_gpio_init();
    }
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->pin),
        .mode = map_mode(config->mode),
        .pull_up_en = (config->pull == HAL_GPIO_PULL_UP || config->pull == HAL_GPIO_PULL_UP_DOWN),
        .pull_down_en = (config->pull == HAL_GPIO_PULL_DOWN || config->pull == HAL_GPIO_PULL_UP_DOWN),
        .intr_type = map_intr_type(config->intr_type),
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %ld: %s", (long)config->pin, esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }
    
    /* Set drive strength */
    gpio_set_drive_capability((gpio_num_t)config->pin, map_drive(config->drive));
    
    /* Configure interrupt if handler provided */
    if (config->isr_handler && config->intr_type != HAL_GPIO_INTR_DISABLE) {
        hal_gpio_set_interrupt(config->pin, config->intr_type, config->isr_handler, config->isr_arg);
    }
    
    /* Update state */
    s_pin_states[config->pin].configured = true;
    s_pin_states[config->pin].mode = config->mode;
    
    HAL_LOGD("Configured GPIO %ld: mode=%d, pull=%d", (long)config->pin, config->mode, config->pull);
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
    
    esp_err_t err = gpio_set_level((gpio_num_t)pin, level ? 1 : 0);
    if (err != ESP_OK) {
        return HAL_ERR_FAILED;
    }
    
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
    
    return gpio_get_level((gpio_num_t)pin) != 0;
}

hal_err_t hal_gpio_toggle(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    bool current = hal_gpio_get_level(pin);
    hal_err_t err = hal_gpio_set_level(pin, !current);
    
#if HAL_ENABLE_STATS
    if (err == HAL_OK) {
        s_pin_states[pin].stats.toggle_count++;
    }
#endif
    
    return err;
}

hal_err_t hal_gpio_set_drive(hal_gpio_pin_t pin, hal_gpio_drive_t drive) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    esp_err_t err = gpio_set_drive_capability((gpio_num_t)pin, map_drive(drive));
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_gpio_set_interrupt(hal_gpio_pin_t pin, hal_gpio_intr_type_t type,
                                  hal_isr_callback_t handler, void* arg) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    /* Install ISR service if not already done */
    if (!s_isr_service_installed) {
        esp_err_t err = gpio_install_isr_service(0);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(err));
            return HAL_ERR_FAILED;
        }
        s_isr_service_installed = true;
    }
    
    /* Store handler info */
    s_pin_states[pin].isr_handler = handler;
    s_pin_states[pin].isr_arg = arg;
    
    /* Configure interrupt type */
    gpio_set_intr_type((gpio_num_t)pin, map_intr_type(type));
    
    /* Add or remove ISR handler */
    if (handler && type != HAL_GPIO_INTR_DISABLE) {
        gpio_isr_handler_add((gpio_num_t)pin, gpio_isr_handler, (void*)(uintptr_t)pin);
    } else {
        gpio_isr_handler_remove((gpio_num_t)pin);
    }
    
    return HAL_OK;
}

hal_err_t hal_gpio_intr_enable(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    esp_err_t err = gpio_intr_enable((gpio_num_t)pin);
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_gpio_intr_disable(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    esp_err_t err = gpio_intr_disable((gpio_num_t)pin);
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_gpio_reset(hal_gpio_pin_t pin) {
    if (pin < 0 || pin >= HAL_GPIO_MAX_PINS) {
        return HAL_ERR_INVALID_ARG;
    }
    
    gpio_reset_pin((gpio_num_t)pin);
    
    memset(&s_pin_states[pin], 0, sizeof(gpio_pin_state_t));
    
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

#endif /* HAL_PLATFORM_ESP_IDF */