/**
 * Periphery Handler Implementation - Auto-generated from Jinja template
 * 
 * Central IO management with FreeRTOS task for periodic updates.
 */

#include "periphery_handler.h"
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_sync.h"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include <string.h>

OSAL_LOG_MODULE_REGISTER(periphery_handler)

//=============================================================================
// Internal State
//=============================================================================

typedef struct {
    periphery_channel_config_t config;
    periphery_channel_data_t data;
    uint32_t last_update_ms;
    bool registered;
} io_channel_t;

static io_channel_t s_channels[PERIPHERY_HANDLER_MAX_CHANNELS];
static size_t s_channel_count = 0;
static SemaphoreHandle_t s_mutex = NULL;
static TaskHandle_t s_task_handle = NULL;
static bool s_running = false;
static periphery_callback_t s_global_callback = NULL;

//=============================================================================
// Helper Functions
//=============================================================================

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static io_channel_t* find_channel_by_name(const char *name) {
    for (size_t i = 0; i < PERIPHERY_HANDLER_MAX_CHANNELS; i++) {
        if (s_channels[i].registered && 
            strcmp(s_channels[i].config.name, name) == 0) {
            return &s_channels[i];
        }
    }
    return NULL;
}

static bool values_equal(io_data_type_t type, periphery_value_t a, periphery_value_t b, float threshold) {
    switch (type) {
        case PERIPHERY_DATA_BOOL:
            return a.b == b.b;
        case PERIPHERY_DATA_INT8:
            return a.i8 == b.i8;
        case PERIPHERY_DATA_INT16:
            return a.i16 == b.i16;
        case PERIPHERY_DATA_INT32:
            return a.i32 == b.i32;
        case PERIPHERY_DATA_UINT8:
            return a.u8 == b.u8;
        case PERIPHERY_DATA_UINT16:
            return a.u16 == b.u16;
        case PERIPHERY_DATA_UINT32:
            return a.u32 == b.u32;
        case PERIPHERY_DATA_FLOAT:
            if (threshold > 0) {
                return (a.f - b.f) < threshold && (b.f - a.f) < threshold;
            }
            return a.f == b.f;
        case PERIPHERY_DATA_DOUBLE:
            if (threshold > 0) {
                return (a.d - b.d) < threshold && (b.d - a.d) < threshold;
            }
            return a.d == b.d;
        default:
            return false;
    }
}

//=============================================================================
// Periphery Handler Task
//=============================================================================

static void periphery_handler_task(void *arg) {
    OSAL_LOGI("Periphery Handler task started");
    
    while (s_running) {
        uint32_t now = get_time_ms();
        uint32_t min_sleep = 1000;  // Max sleep time
        
        if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (size_t i = 0; i < PERIPHERY_HANDLER_MAX_CHANNELS; i++) {
                io_channel_t *ch = &s_channels[i];
                
                if (!ch->registered || ch->config.update_interval_ms == 0) {
                    continue;
                }
                
                // Check if update is due
                uint32_t elapsed = now - ch->last_update_ms;
                if (elapsed >= ch->config.update_interval_ms) {
                    // Time to update
                    if (ch->config.read_func != NULL) {
                        periphery_value_t new_value;
                        esp_err_t ret = ch->config.read_func(i, &new_value);
                        
                        if (ret == ESP_OK) {
                            bool changed = !values_equal(
                                ch->config.data_type, 
                                ch->data.value, 
                                new_value,
                                ch->config.change_threshold
                            );
                            
                            ch->data.value = new_value;
                            ch->data.timestamp_ms = now;
                            ch->data.valid = true;
                            ch->last_update_ms = now;
                            
                            // Trigger callbacks if value changed (or always if not update_on_change)
                            if (!ch->config.update_on_change || changed) {
                                if (ch->config.callback) {
                                    ch->config.callback(i, &ch->data);
                                }
                                if (s_global_callback) {
                                    s_global_callback(i, &ch->data);
                                }
                            }
                        } else {
                            ch->data.valid = false;
                            OSAL_LOGW("Failed to read channel '%s': %s", 
                                     ch->config.name, esp_err_to_name(ret));
                        }
                    }
                } else {
                    // Calculate time until next update
                    uint32_t remaining = ch->config.update_interval_ms - elapsed;
                    if (remaining < min_sleep) {
                        min_sleep = remaining;
                    }
                }
            }
            xSemaphoreGive(s_mutex);
        }
        
        // Sleep until next update is due
        if (min_sleep < PERIPHERY_HANDLER_MIN_INTERVAL_MS) {
            min_sleep = PERIPHERY_HANDLER_MIN_INTERVAL_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(min_sleep));
    }
    
    OSAL_LOGI("Periphery Handler task stopped");
    vTaskDelete(NULL);
}

//=============================================================================
// Public API
//=============================================================================

esp_err_t periphery_handler_init(void) {
    if (s_mutex != NULL) {
        OSAL_LOGW("Periphery Handler already initialized");
        return ESP_OK;
    }
    
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        OSAL_LOGE("Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    memset(s_channels, 0, sizeof(s_channels));
    s_channel_count = 0;
    s_running = false;
    s_global_callback = NULL;
    
    OSAL_LOGI("Periphery Handler initialized (max %d channels)", PERIPHERY_HANDLER_MAX_CHANNELS);
    return ESP_OK;
}

void periphery_handler_deinit(void) {
    periphery_handler_stop();
    
    if (s_mutex != NULL) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }
    
    memset(s_channels, 0, sizeof(s_channels));
    s_channel_count = 0;
}

esp_err_t periphery_handler_register_channel(const periphery_channel_config_t *config, uint8_t *channel_id) {
    if (config == NULL || config->name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Check for duplicate name
    if (find_channel_by_name(config->name) != NULL) {
        xSemaphoreGive(s_mutex);
        OSAL_LOGE("Channel '%s' already registered", config->name);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Find free slot
    int slot = -1;
    for (size_t i = 0; i < PERIPHERY_HANDLER_MAX_CHANNELS; i++) {
        if (!s_channels[i].registered) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        xSemaphoreGive(s_mutex);
        OSAL_LOGE("No free channel slots");
        return ESP_ERR_NO_MEM;
    }
    
    // Register channel
    io_channel_t *ch = &s_channels[slot];
    memcpy(&ch->config, config, sizeof(periphery_channel_config_t));
    ch->data.name = config->name;
    ch->data.type = config->type;
    ch->data.data_type = config->data_type;
    ch->data.unit = config->unit;
    ch->data.valid = false;
    ch->data.timestamp_ms = 0;
    ch->last_update_ms = 0;
    ch->registered = true;
    s_channel_count++;
    
    if (channel_id != NULL) {
        *channel_id = slot;
    }
    
    xSemaphoreGive(s_mutex);
    
    OSAL_LOGI("Registered channel '%s' (ID=%d, type=%d, interval=%lums)", 
             config->name, slot, config->type, config->update_interval_ms);
    return ESP_OK;
}

esp_err_t periphery_handler_unregister_channel(uint8_t channel_id) {
    if (channel_id >= PERIPHERY_HANDLER_MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (!s_channels[channel_id].registered) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    s_channels[channel_id].registered = false;
    s_channel_count--;
    
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

int periphery_handler_get_channel_by_name(const char *name) {
    if (name == NULL) {
        return -1;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }
    
    for (size_t i = 0; i < PERIPHERY_HANDLER_MAX_CHANNELS; i++) {
        if (s_channels[i].registered && 
            strcmp(s_channels[i].config.name, name) == 0) {
            xSemaphoreGive(s_mutex);
            return i;
        }
    }
    
    xSemaphoreGive(s_mutex);
    return -1;
}

esp_err_t periphery_handler_read(uint8_t channel_id, periphery_channel_data_t *data) {
    if (channel_id >= PERIPHERY_HANDLER_MAX_CHANNELS || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    if (!s_channels[channel_id].registered) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    memcpy(data, &s_channels[channel_id].data, sizeof(periphery_channel_data_t));
    
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t periphery_handler_read_by_name(const char *name, periphery_channel_data_t *data) {
    int id = periphery_handler_get_channel_by_name(name);
    if (id < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    return periphery_handler_read(id, data);
}

esp_err_t periphery_handler_write(uint8_t channel_id, periphery_value_t value) {
    if (channel_id >= PERIPHERY_HANDLER_MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    io_channel_t *ch = &s_channels[channel_id];
    if (!ch->registered) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    if (ch->config.write_func == NULL) {
        xSemaphoreGive(s_mutex);
        OSAL_LOGE("Channel '%s' is read-only", ch->config.name);
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    esp_err_t ret = ch->config.write_func(channel_id, value);
    if (ret == ESP_OK) {
        ch->data.value = value;
        ch->data.timestamp_ms = get_time_ms();
        ch->data.valid = true;
        
        // Trigger callbacks
        if (ch->config.callback) {
            ch->config.callback(channel_id, &ch->data);
        }
        if (s_global_callback) {
            s_global_callback(channel_id, &ch->data);
        }
    }
    
    xSemaphoreGive(s_mutex);
    return ret;
}

esp_err_t periphery_handler_write_by_name(const char *name, periphery_value_t value) {
    int id = periphery_handler_get_channel_by_name(name);
    if (id < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    return periphery_handler_write(id, value);
}

esp_err_t periphery_handler_force_update(uint8_t channel_id) {
    if (channel_id >= PERIPHERY_HANDLER_MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    io_channel_t *ch = &s_channels[channel_id];
    if (!ch->registered) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    
    if (ch->config.read_func == NULL) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    periphery_value_t new_value;
    esp_err_t ret = ch->config.read_func(channel_id, &new_value);
    if (ret == ESP_OK) {
        ch->data.value = new_value;
        ch->data.timestamp_ms = get_time_ms();
        ch->data.valid = true;
        ch->last_update_ms = ch->data.timestamp_ms;
    }
    
    xSemaphoreGive(s_mutex);
    return ret;
}

esp_err_t periphery_handler_start(uint8_t task_priority, uint32_t stack_size) {
    if (s_running) {
        OSAL_LOGW("Periphery Handler task already running");
        return ESP_OK;
    }
    
    s_running = true;
    
    BaseType_t ret = xTaskCreate(
        periphery_handler_task,
        "periphery_handler",
        stack_size,
        NULL,
        task_priority,
        &s_task_handle
    );
    
    if (ret != pdPASS) {
        s_running = false;
        OSAL_LOGE("Failed to create Periphery Handler task");
        return ESP_ERR_NO_MEM;
    }
    
    OSAL_LOGI("Periphery Handler task started (priority=%d, stack=%lu)", 
             task_priority, stack_size);
    return ESP_OK;
}

void periphery_handler_stop(void) {
    if (!s_running) {
        return;
    }
    
    s_running = false;
    
    // Wait for task to finish
    if (s_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_task_handle = NULL;
    }
    
    OSAL_LOGI("Periphery Handler task stopped");
}

void periphery_handler_set_global_callback(periphery_callback_t callback) {
    s_global_callback = callback;
}

int periphery_handler_get_all_channels(periphery_channel_data_t *data, int max_channels) {
    if (data == NULL || max_channels <= 0) {
        return 0;
    }
    
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }
    
    int count = 0;
    for (size_t i = 0; i < PERIPHERY_HANDLER_MAX_CHANNELS && count < max_channels; i++) {
        if (s_channels[i].registered) {
            memcpy(&data[count], &s_channels[i].data, sizeof(periphery_channel_data_t));
            count++;
        }
    }
    
    xSemaphoreGive(s_mutex);
    return count;
}

int periphery_handler_get_channel_count(void) {
    return s_channel_count;
}
