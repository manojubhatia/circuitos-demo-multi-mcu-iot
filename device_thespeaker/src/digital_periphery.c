/**
 * Digital I/O Module Implementation - Auto-generated from Jinja template
 * 
 * Provides digital GPIO operations with automatic IO Handler registration.
 */

#include "digital_periphery.h"
#include "pal/pal_gpio.h"
#include "osal/osal_log.h"
#include <string.h>

OSAL_LOG_MODULE_REGISTER(digital_periphery)

//=============================================================================
// Internal State
//=============================================================================

#define MAX_DIGITAL_IO_CHANNELS 16

typedef struct {
    const digital_periphery_config_t *config;
    uint8_t periphery_handler_channel_id;
    bool current_state;
} digital_periphery_channel_t;

static digital_periphery_channel_t s_channels[MAX_DIGITAL_IO_CHANNELS];
static size_t s_channel_count = 0;

//=============================================================================
// IO Handler Callbacks
//=============================================================================

/**
 * @brief Read callback for IO Handler
 */
static esp_err_t digital_periphery_read_callback(uint8_t channel_id, periphery_value_t *value) {
    // Find our channel by IO handler ID
    for (size_t i = 0; i < s_channel_count; i++) {
        if (s_channels[i].periphery_handler_channel_id == channel_id) {
            const digital_periphery_config_t *cfg = s_channels[i].config;
            int level = gpio_get_level(cfg->pin);
            bool state = cfg->active_low ? (level == 0) : (level != 0);
            s_channels[i].current_state = state;
            value->b = state;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Write callback for IO Handler
 */
static esp_err_t digital_periphery_write_callback(uint8_t channel_id, periphery_value_t value) {
    // Find our channel by IO handler ID
    for (size_t i = 0; i < s_channel_count; i++) {
        if (s_channels[i].periphery_handler_channel_id == channel_id) {
            const digital_periphery_config_t *cfg = s_channels[i].config;
            
            if (!cfg->is_output) {
                OSAL_LOGE("Cannot write to input pin '%s'", cfg->name);
                return ESP_ERR_INVALID_STATE;
            }
            
            bool level = value.b;
            if (cfg->active_low) level = !level;
            
            gpio_set_level(cfg->pin, level ? 1 : 0);
            s_channels[i].current_state = value.b;
            
            OSAL_LOGD("Set '%s' (GPIO %d) to %d", cfg->name, cfg->pin, value.b);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

//=============================================================================
// Public API
//=============================================================================

esp_err_t digital_periphery_register_with_handler(const digital_periphery_config_t *configs, size_t count) {
    if (configs == NULL || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (s_channel_count + count > MAX_DIGITAL_IO_CHANNELS) {
        OSAL_LOGE("Too many digital IO channels (max %d)", MAX_DIGITAL_IO_CHANNELS);
        return ESP_ERR_NO_MEM;
    }
    
    for (size_t i = 0; i < count; i++) {
        const digital_periphery_config_t *cfg = &configs[i];
        
        // Configure GPIO
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << cfg->pin),
            .mode = cfg->is_output ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
            .pull_up_en = cfg->pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = cfg->pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            OSAL_LOGE("Failed to configure GPIO %d for '%s': %s", 
                     cfg->pin, cfg->name, esp_err_to_name(ret));
            return ret;
        }
        
        // Set initial state for outputs
        if (cfg->is_output) {
            bool level = cfg->initial_state;
            if (cfg->active_low) level = !level;
            gpio_set_level(cfg->pin, level ? 1 : 0);
        }
        
        // Register with IO Handler
        io_channel_config_t io_cfg = {
            .name = cfg->name,
            .type = cfg->is_output ? IO_TYPE_DIGITAL_OUTPUT : IO_TYPE_DIGITAL_INPUT,
            .data_type = IO_DATA_BOOL,
            .unit = "",
            .update_interval_ms = cfg->is_output ? 0 : 50,  // Inputs polled at 50ms
            .update_on_change = true,
            .read_func = digital_periphery_read_callback,
            .write_func = cfg->is_output ? digital_periphery_write_callback : NULL,
            .hw.digital = {
                .pin = cfg->pin,
                .active_low = cfg->active_low,
                .pull_up = cfg->pull_up,
                .pull_down = cfg->pull_down
            }
        };
        
        uint8_t channel_id;
        ret = periphery_handler_register_channel(&io_cfg, &channel_id);
        if (ret != ESP_OK) {
            OSAL_LOGE("Failed to register '%s' with IO Handler: %s", 
                     cfg->name, esp_err_to_name(ret));
            return ret;
        }
        
        // Store channel info
        s_channels[s_channel_count].config = cfg;
        s_channels[s_channel_count].periphery_handler_channel_id = channel_id;
        s_channels[s_channel_count].current_state = cfg->initial_state;
        s_channel_count++;
        
        OSAL_LOGI("Registered '%s' on GPIO %d as %s (IO Handler channel %d)", 
                 cfg->name, cfg->pin, cfg->is_output ? "OUTPUT" : "INPUT", channel_id);
    }
    
    return ESP_OK;
}

esp_err_t digital_periphery_write(const char *name, bool state) {
    periphery_value_t value = { .b = state };
    return periphery_handler_write_by_name(name, value);
}

esp_err_t digital_periphery_read(const char *name, bool *state) {
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    io_channel_data_t data;
    esp_err_t ret = periphery_handler_read_by_name(name, &data);
    if (ret == ESP_OK) {
        *state = data.value.b;
    }
    return ret;
}

esp_err_t digital_periphery_toggle(const char *name) {
    bool current;
    esp_err_t ret = digital_periphery_read(name, &current);
    if (ret != ESP_OK) {
        return ret;
    }
    return digital_periphery_write(name, !current);
}

int digital_periphery_get_pin(const char *name) {
    for (size_t i = 0; i < s_channel_count; i++) {
        if (strcmp(s_channels[i].config->name, name) == 0) {
            return s_channels[i].config->pin;
        }
    }
    return -1;
}
