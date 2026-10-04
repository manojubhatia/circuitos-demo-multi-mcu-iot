/**
 * @file protocol_handler.c
 * @brief Protocol Handler Implementation
 * 
 * Role: sender
 * Transport: spi
 * 
 * Auto-generated
 */

#include "protocol_handler.h"
#include "transport_spi.h"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_sync.h"

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_crc.h"
#endif
#include <stdint.h>
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_sync.h"
#include <string.h>

OSAL_LOG_MODULE_REGISTER(PROTO_HANDLER)

//=============================================================================
// Internal State
//=============================================================================

static struct {
    bool initialized;
    bool running;
    bool is_sender;
    bool is_relay;
    uint32_t send_interval_ms;
    
    // Signals storage (for both sending and receiving)
    protocol_signal_t signals[PROTOCOL_MAX_SIGNALS];
    int signal_count;
    SemaphoreHandle_t signals_mutex;
    
    // Received signals storage (separate for relay MCUs)
    protocol_signal_t rx_signals[PROTOCOL_MAX_SIGNALS];
    int rx_signal_count;
    
    // Task
    TaskHandle_t task_handle;
    
    // Callbacks
    void (*on_data_received)(const protocol_signal_t *signals, uint8_t count);
    void (*on_relay_forward)(const protocol_signal_t *signals, uint8_t count);
    
    // Statistics
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    
    // Sequence number
    uint16_t sequence;
} s_state = {0};

//=============================================================================
// Helper Functions
//=============================================================================

static uint32_t get_timestamp_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static uint16_t calculate_checksum(const void *data, size_t len) {
    return (uint16_t)esp_crc16_le(0, (const uint8_t *)data, len);
}

static int find_signal_by_name(const char *name) {
    for (int i = 0; i < s_state.signal_count; i++) {
        if (strncmp(s_state.signals[i].name, name, sizeof(s_state.signals[i].name)) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_rx_signal_by_name(const char *name) {
    for (int i = 0; i < s_state.rx_signal_count; i++) {
        if (strncmp(s_state.rx_signals[i].name, name, sizeof(s_state.rx_signals[i].name)) == 0) {
            return i;
        }
    }
    return -1;
}

//=============================================================================
// Sender Implementation
//=============================================================================

static esp_err_t send_signal_data(void) {
    if (s_state.signal_count == 0) {
        return ESP_OK;
    }
    
    uint8_t buffer[sizeof(sensor_data_msg_t) + PROTOCOL_MAX_SIGNALS * sizeof(protocol_signal_t)];
    sensor_data_msg_t *msg = (sensor_data_msg_t *)buffer;
    
    msg->header.magic = PROTOCOL_MAGIC;
    msg->header.version = PROTOCOL_VERSION;
    msg->header.msg_type = MSG_TYPE_SENSOR_DATA;
    msg->header.sequence = s_state.sequence++;
    msg->header.timestamp_ms = get_timestamp_ms();
    
    if (xSemaphoreTake(s_state.signals_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        msg->signal_count = s_state.signal_count;
        memcpy(buffer + sizeof(sensor_data_msg_t), s_state.signals, 
               s_state.signal_count * sizeof(protocol_signal_t));
        xSemaphoreGive(s_state.signals_mutex);
    } else {
        return ESP_ERR_TIMEOUT;
    }
    
    size_t payload_len = sizeof(uint8_t) + s_state.signal_count * sizeof(protocol_signal_t);
    msg->header.payload_len = payload_len;
    msg->header.checksum = 0;
    
    size_t total_len = sizeof(protocol_header_t) + payload_len;
    msg->header.checksum = calculate_checksum(buffer, total_len);
    
    esp_err_t ret = transport_spi_send(buffer, total_len);
    
    if (ret == ESP_OK) {
        s_state.tx_count++;
        OSAL_LOGD("Sent %d signals (seq=%d)", msg->signal_count, msg->header.sequence);
    } else {
        s_state.error_count++;
        OSAL_LOGE("Failed to send: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

int protocol_handler_register_signal(const char *name, signal_type_t type) {
    if (!s_state.initialized) {
        return -1;
    }
    
    if (s_state.signal_count >= PROTOCOL_MAX_SIGNALS) {
        OSAL_LOGE("Max signals reached");
        return -1;
    }
    
    if (xSemaphoreTake(s_state.signals_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }
    
    int index = s_state.signal_count;
    protocol_signal_t *sig = &s_state.signals[index];
    
    strncpy(sig->name, name, sizeof(sig->name) - 1);
    sig->name[sizeof(sig->name) - 1] = '\0';
    sig->type = type;
    memset(&sig->value, 0, sizeof(sig->value));
    sig->last_update = 0;
    
    s_state.signal_count++;
    
    xSemaphoreGive(s_state.signals_mutex);
    
    OSAL_LOGI("Registered signal '%s' (type=%d, index=%d)", name, type, index);
    return index;
}

esp_err_t protocol_handler_update_signal(int index, signal_value_t value) {
    if (!s_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (index < 0 || index >= s_state.signal_count) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_state.signals_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    s_state.signals[index].value = value;
    s_state.signals[index].last_update = get_timestamp_ms();
    
    xSemaphoreGive(s_state.signals_mutex);
    
    return ESP_OK;
}

esp_err_t protocol_handler_update_signal_by_name(const char *name, signal_value_t value) {
    int index = find_signal_by_name(name);
    if (index < 0) {
        // Auto-register if not found
        signal_type_t type = SIGNAL_TYPE_BOOL;  // Default
        index = protocol_handler_register_signal(name, type);
        if (index < 0) {
            return ESP_ERR_NOT_FOUND;
        }
    }
    return protocol_handler_update_signal(index, value);
}

esp_err_t protocol_handler_send_now(void) {
    return send_signal_data();
}

esp_err_t protocol_handler_send_bool(const char *name, bool value) {
    signal_value_t v = { .b = value };
    int index = find_signal_by_name(name);
    if (index < 0) {
        index = protocol_handler_register_signal(name, SIGNAL_TYPE_BOOL);
        if (index < 0) return ESP_ERR_NO_MEM;
    }
    esp_err_t ret = protocol_handler_update_signal(index, v);
    if (ret == ESP_OK) {
        ret = protocol_handler_send_now();
    }
    return ret;
}

esp_err_t protocol_handler_send_int(const char *name, int32_t value) {
    signal_value_t v = { .i = value };
    int index = find_signal_by_name(name);
    if (index < 0) {
        index = protocol_handler_register_signal(name, SIGNAL_TYPE_INT32);
        if (index < 0) return ESP_ERR_NO_MEM;
    }
    esp_err_t ret = protocol_handler_update_signal(index, v);
    if (ret == ESP_OK) {
        ret = protocol_handler_send_now();
    }
    return ret;
}

esp_err_t protocol_handler_send_float(const char *name, float value) {
    signal_value_t v = { .f = value };
    int index = find_signal_by_name(name);
    if (index < 0) {
        index = protocol_handler_register_signal(name, SIGNAL_TYPE_FLOAT);
        if (index < 0) return ESP_ERR_NO_MEM;
    }
    esp_err_t ret = protocol_handler_update_signal(index, v);
    if (ret == ESP_OK) {
        ret = protocol_handler_send_now();
    }
    return ret;
}

//=============================================================================
// Protocol Handler Task
//=============================================================================

static void protocol_handler_task(void *arg) {
    uint8_t rx_buffer[sizeof(protocol_header_t) + PROTOCOL_MAX_PAYLOAD];
    
    OSAL_LOGI("Protocol handler task started (sender=%d, relay=%d)", 
             s_state.is_sender, s_state.is_relay);
    
    uint32_t last_send_time = 0;
    
    while (s_state.running) {
        // Sender: periodically send data
        if (s_state.is_sender && !s_state.is_relay) {
            uint32_t now = get_timestamp_ms();
            if ((now - last_send_time) >= s_state.send_interval_ms) {
                send_signal_data();
                last_send_time = now;
            }
        }

        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    OSAL_LOGI("Protocol handler task stopped");
    vTaskDelete(NULL);
}

//=============================================================================
// Public API
//=============================================================================

esp_err_t protocol_handler_init(const protocol_handler_config_t *config) {
    if (s_state.initialized) {
        OSAL_LOGW("Already initialized");
        return ESP_OK;
    }
    
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    OSAL_LOGI("Initializing protocol handler (sender=%d, relay=%d)", 
             config->is_sender, config->is_relay);
    
    s_state.is_sender = config->is_sender;
    s_state.is_relay = config->is_relay;
    s_state.send_interval_ms = config->send_interval_ms > 0 ? config->send_interval_ms : 500;
    s_state.on_data_received = config->on_data_received;
    s_state.on_relay_forward = config->on_relay_forward;
    s_state.signal_count = 0;
    s_state.rx_signal_count = 0;
    s_state.sequence = 0;
    s_state.tx_count = 0;
    s_state.rx_count = 0;
    s_state.error_count = 0;
    
    s_state.signals_mutex = xSemaphoreCreateMutex();
    if (!s_state.signals_mutex) {
        OSAL_LOGE("Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }
    
    s_state.initialized = true;
    OSAL_LOGI("Protocol handler initialized");
    return ESP_OK;
}

void protocol_handler_deinit(void) {
    if (!s_state.initialized) {
        return;
    }
    
    protocol_handler_stop();
    
    if (s_state.signals_mutex) {
        vSemaphoreDelete(s_state.signals_mutex);
        s_state.signals_mutex = NULL;
    }
    
    s_state.initialized = false;
    OSAL_LOGI("Protocol handler deinitialized");
}

esp_err_t protocol_handler_start(void) {
    if (!s_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_state.running) {
        return ESP_OK;
    }
    
    s_state.running = true;
    
    BaseType_t ret = xTaskCreate(
        protocol_handler_task,
        "proto_handler",
        4096,
        NULL,
        5,
        &s_state.task_handle
    );
    
    if (ret != pdPASS) {
        s_state.running = false;
        OSAL_LOGE("Failed to create task");
        return ESP_ERR_NO_MEM;
    }
    
    OSAL_LOGI("Protocol handler started");
    return ESP_OK;
}

void protocol_handler_stop(void) {
    if (!s_state.running) {
        return;
    }
    
    s_state.running = false;
    
    if (s_state.task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_state.task_handle = NULL;
    }
    
    OSAL_LOGI("Protocol handler stopped");
}

void protocol_handler_get_stats(uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count) {
    if (tx_count) *tx_count = s_state.tx_count;
    if (rx_count) *rx_count = s_state.rx_count;
    if (error_count) *error_count = s_state.error_count;
}