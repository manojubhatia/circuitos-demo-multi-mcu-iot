/**
 * @file spi_slave_comm.c
 * @brief SPI Slave Communication - Bidirectional Implementation
 * 
 * SPI Slave responds to master transactions with sensor data.
 * Receives configuration commands from master.
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#include "spi_slave_comm.h"
#include <string.h>
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_log.h"
#include "pal/pal_spi.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "driver/spi_slave.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "osal/osal_log.h"

OSAL_LOG_MODULE_REGISTER(spi_slave)

/* Module state */
static bool s_initialized = false;
static uint32_t s_sequence_num = 0;
static uint32_t s_tx_interval_ms = SPI_COMM_DEFAULT_INTERVAL;

/* Statistics */
static uint32_t s_transactions = 0;
static uint32_t s_errors = 0;

/* Callback */
static spi_slave_config_callback_t s_config_callback = NULL;

/* Sensor buffer */
static spi_sensor_reading_t s_sensor_buffer[SPI_COMM_MAX_SENSORS];
static uint8_t s_sensor_count = 0;

/* Pre-prepared response buffer */
static uint8_t s_response_buffer[SPI_COMM_MAX_PACKET_SIZE + 6];
static size_t s_response_len = 0;

/* Transaction buffers */
static WORD_ALIGNED_ATTR uint8_t s_rx_buffer[SPI_COMM_MAX_PACKET_SIZE + 6];
static WORD_ALIGNED_ATTR uint8_t s_tx_buffer[SPI_COMM_MAX_PACKET_SIZE + 6];

/* Serialize sensor packet */
static size_t serialize_sensor_packet(uint8_t *buffer, size_t max_len) {
    if (s_sensor_count == 0 || max_len < 10) return 0;
    
    size_t offset = 0;
    buffer[offset++] = SPI_COMM_MCU_ID;
    buffer[offset++] = (s_sequence_num >> 24) & 0xFF;
    buffer[offset++] = (s_sequence_num >> 16) & 0xFF;
    buffer[offset++] = (s_sequence_num >> 8) & 0xFF;
    buffer[offset++] = s_sequence_num & 0xFF;
    buffer[offset++] = s_sensor_count;
    
    for (int i = 0; i < s_sensor_count && i < SPI_COMM_MAX_SENSORS; i++) {
        if (offset + 10 > max_len) break;
        
        const spi_sensor_reading_t *reading = &s_sensor_buffer[i];
        buffer[offset++] = reading->sensor_id;
        buffer[offset++] = reading->sensor_type;
        
        uint32_t float_bits;
        memcpy(&float_bits, &reading->value, sizeof(float));
        buffer[offset++] = (float_bits >> 24) & 0xFF;
        buffer[offset++] = (float_bits >> 16) & 0xFF;
        buffer[offset++] = (float_bits >> 8) & 0xFF;
        buffer[offset++] = float_bits & 0xFF;
        
        buffer[offset++] = (reading->timestamp_ms >> 24) & 0xFF;
        buffer[offset++] = (reading->timestamp_ms >> 16) & 0xFF;
        buffer[offset++] = (reading->timestamp_ms >> 8) & 0xFF;
        buffer[offset++] = reading->timestamp_ms & 0xFF;
    }
    
    return offset;
}

/* Handle received command */
static void handle_config_command(const uint8_t *payload, size_t len) {
    if (len < 6) return;
    
    spi_config_command_t cmd;
    cmd.cmd_type = payload[0];
    cmd.sensor_id = payload[1];
    cmd.value = ((uint32_t)payload[2] << 24) |
                ((uint32_t)payload[3] << 16) |
                ((uint32_t)payload[4] << 8) |
                (uint32_t)payload[5];
    
    OSAL_LOGI("Config command: type=%d, sensor=%d, value=%lu",
             cmd.cmd_type, cmd.sensor_id, cmd.value);
    
    switch (cmd.cmd_type) {
        case SPI_CONFIG_CMD_SET_INTERVAL:
            if (cmd.value >= 100 && cmd.value <= 60000) {
                s_tx_interval_ms = cmd.value;
                OSAL_LOGI("Interval changed to %lu ms", s_tx_interval_ms);
            }
            break;
        case SPI_CONFIG_CMD_RESET:
            s_tx_interval_ms = SPI_COMM_DEFAULT_INTERVAL;
            OSAL_LOGI("Reset to defaults");
            break;
        default:
            break;
    }
    
    if (s_config_callback) {
        s_config_callback(&cmd);
    }
}

/* SPI transaction callback */
static void IRAM_ATTR spi_post_trans_cb(spi_slave_transaction_t *trans) {
    // Called after each transaction
}

/* SPI slave task */
static void spi_slave_task(void *arg) {
    OSAL_LOGI("SPI Slave task started");
    
    while (1) {
        spi_slave_transaction_t trans = {
            .length = sizeof(s_rx_buffer) * 8,
            .tx_buffer = s_tx_buffer,
            .rx_buffer = s_rx_buffer,
        };
        
        // Prepare response data
        memcpy(s_tx_buffer, s_response_buffer, s_response_len);
        
        esp_err_t ret = spi_slave_transmit(SPI2_HOST, &trans, portMAX_DELAY);
        if (ret != ESP_OK) {
            s_errors++;
            continue;
        }
        
        s_transactions++;
        
        // Process received data
        if (s_rx_buffer[0] == SPI_COMM_START_MARKER) {
            uint8_t frame_type = s_rx_buffer[1];
            uint16_t payload_len = ((uint16_t)s_rx_buffer[2] << 8) | s_rx_buffer[3];
            
            switch (frame_type) {
                case SPI_FRAME_TYPE_REQUEST:
                    OSAL_LOGD("Data request from master");
                    s_sequence_num++;
                    spi_slave_prepare_response();
                    break;
                    
                case SPI_FRAME_TYPE_CONFIG:
                    handle_config_command(&s_rx_buffer[4], payload_len);
                    break;
                    
                default:
                    break;
            }
        }
    }
}

/* Public API */

esp_err_t spi_slave_init(void) {
    if (s_initialized) return ESP_OK;
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_COMM_MOSI_PIN,
        .miso_io_num = SPI_COMM_MISO_PIN,
        .sclk_io_num = SPI_COMM_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    
    spi_slave_interface_config_t slave_cfg = {
        .mode = 0,
        .spics_io_num = SPI_COMM_CS_PIN,
        .queue_size = 3,
        .post_trans_cb = spi_post_trans_cb,
    };
    
    esp_err_t ret = spi_slave_initialize(SPI2_HOST, &bus_cfg, &slave_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        OSAL_LOGE("Failed to initialize SPI slave: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Prepare initial response
    spi_slave_prepare_response();
    
    // Start slave task
    xTaskCreate(spi_slave_task, "spi_slave", 4096, NULL, 10, NULL);
    
    s_initialized = true;
    OSAL_LOGI("=== SPI SLAVE Initialized ===");
    OSAL_LOGI("MOSI=%d, MISO=%d, SCK=%d, CS=%d",
             SPI_COMM_MOSI_PIN, SPI_COMM_MISO_PIN, SPI_COMM_SCK_PIN, SPI_COMM_CS_PIN);
    
    return ESP_OK;
}

esp_err_t spi_slave_deinit(void) {
    if (!s_initialized) return ESP_OK;
    spi_slave_free(SPI2_HOST);
    s_initialized = false;
    return ESP_OK;
}

void spi_slave_set_config_callback(spi_slave_config_callback_t callback) {
    s_config_callback = callback;
}

void spi_slave_update_sensor(uint8_t sensor_id, spi_sensor_type_t sensor_type, float value) {
    for (int i = 0; i < s_sensor_count; i++) {
        if (s_sensor_buffer[i].sensor_id == sensor_id) {
            s_sensor_buffer[i].sensor_type = sensor_type;
            s_sensor_buffer[i].value = value;
            s_sensor_buffer[i].timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            return;
        }
    }
    
    if (s_sensor_count < SPI_COMM_MAX_SENSORS) {
        s_sensor_buffer[s_sensor_count].sensor_id = sensor_id;
        s_sensor_buffer[s_sensor_count].sensor_type = sensor_type;
        s_sensor_buffer[s_sensor_count].value = value;
        s_sensor_buffer[s_sensor_count].timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        s_sensor_count++;
    }
}

void spi_slave_prepare_response(void) {
    s_response_buffer[0] = SPI_COMM_START_MARKER;
    s_response_buffer[1] = SPI_FRAME_TYPE_SENSOR_DATA;
    
    size_t payload_len = serialize_sensor_packet(&s_response_buffer[4], sizeof(s_response_buffer) - 6);
    s_response_buffer[2] = (payload_len >> 8) & 0xFF;
    s_response_buffer[3] = payload_len & 0xFF;
    
    s_response_len = 4 + payload_len;
}

/* Convenience functions */
void spi_slave_set_inmp441_i2s_mems_microphone(float value) {
    spi_slave_update_sensor(SPI_SENSOR_ID_INMP441_I2S_MEMS_MICROPHONE, SPI_SENSOR_TYPE_GENERIC, value);
}

uint32_t spi_slave_get_interval(void) {
    return s_tx_interval_ms;
}

void spi_slave_get_stats(uint32_t *transactions, uint32_t *errors) {
    if (transactions) *transactions = s_transactions;
    if (errors) *errors = s_errors;
}