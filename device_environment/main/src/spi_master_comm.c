/**
 * @file spi_master_comm.c
 * @brief SPI Master Communication - Bidirectional Implementation
 * 
 * SPI Master controls all transactions. Full-duplex allows simultaneous
 * send/receive in each transaction.
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#include "spi_master_comm.h"
#include <string.h>
#include <math.h>
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "pal/pal_spi.h"
#include "osal/osal_log.h"

#if defined(ESP_PLATFORM) || defined(IDF_VER)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#endif

OSAL_LOG_MODULE_REGISTER(spi_master)

/* Module state */
static bool s_initialized = false;
static spi_device_handle_t s_spi_handle = NULL;
static uint32_t s_cmd_sequence_num = 0;

/* Statistics */
static uint32_t s_transactions = 0;
static uint32_t s_errors = 0;

/* Receiver state */
static spi_master_data_callback_t s_data_callback = NULL;
static uint32_t s_last_update_ms = 0;

/* Sensor value storage */
static float s_joystick_up = NAN;
static float s_joystick_down = NAN;
static float s_joystick_left = NAN;
static float s_joystick_right = NAN;
static float s_joystick_center_press = NAN;
static float s_boot_button = NAN;
static float s_aht10_temperature_humidity_sensor = NAN;

#define MAX_SENSOR_STORAGE 16
static struct {
    uint8_t sensor_id;
    float value;
    bool valid;
} s_sensor_storage[MAX_SENSOR_STORAGE];

/* CRC16 calculation */
static uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return crc;
}

/* Deserialize sensor packet */
static bool deserialize_sensor_packet(const uint8_t *buffer, size_t len, spi_sensor_packet_t *packet) {
    if (!buffer || !packet || len < 6) return false;
    
    size_t offset = 0;
    packet->mcu_id = buffer[offset++];
    packet->sequence_num = ((uint32_t)buffer[offset] << 24) |
                           ((uint32_t)buffer[offset + 1] << 16) |
                           ((uint32_t)buffer[offset + 2] << 8) |
                           (uint32_t)buffer[offset + 3];
    offset += 4;
    packet->sensor_count = buffer[offset++];
    
    if (packet->sensor_count > SPI_COMM_MAX_SENSORS) {
        packet->sensor_count = SPI_COMM_MAX_SENSORS;
    }
    
    for (int i = 0; i < packet->sensor_count; i++) {
        if (offset + 10 > len) break;
        
        spi_sensor_reading_t *reading = &packet->sensors[i];
        reading->sensor_id = buffer[offset++];
        reading->sensor_type = buffer[offset++];
        
        uint32_t float_bits = ((uint32_t)buffer[offset] << 24) |
                              ((uint32_t)buffer[offset + 1] << 16) |
                              ((uint32_t)buffer[offset + 2] << 8) |
                              (uint32_t)buffer[offset + 3];
        memcpy(&reading->value, &float_bits, sizeof(float));
        offset += 4;
        
        reading->timestamp_ms = ((uint32_t)buffer[offset] << 24) |
                                ((uint32_t)buffer[offset + 1] << 16) |
                                ((uint32_t)buffer[offset + 2] << 8) |
                                (uint32_t)buffer[offset + 3];
        offset += 4;
    }
    
    return true;
}

/* Store received sensor values */
static void store_sensor_values(const spi_sensor_packet_t *packet) {
    for (int i = 0; i < packet->sensor_count; i++) {
        const spi_sensor_reading_t *reading = &packet->sensors[i];
        
        switch (reading->sensor_type) {
            default:
                OSAL_LOGI("Sensor[%d]: %.2f", reading->sensor_id, reading->value);
                break;
        }
        
        // Generic storage
        for (int j = 0; j < MAX_SENSOR_STORAGE; j++) {
            if (!s_sensor_storage[j].valid || s_sensor_storage[j].sensor_id == reading->sensor_id) {
                s_sensor_storage[j].sensor_id = reading->sensor_id;
                s_sensor_storage[j].value = reading->value;
                s_sensor_storage[j].valid = true;
                break;
            }
        }
    }
    s_last_update_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/* Public API */

esp_err_t spi_master_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }
    
    memset(s_sensor_storage, 0, sizeof(s_sensor_storage));
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_COMM_MOSI_PIN,
        .miso_io_num = SPI_COMM_MISO_PIN,
        .sclk_io_num = SPI_COMM_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_COMM_MAX_PACKET_SIZE + 10,
    };
    
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        OSAL_LOGE("Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_COMM_CLOCK_SPEED_HZ,
        .mode = 0,
        .spics_io_num = SPI_COMM_CS_PIN,
        .queue_size = 7,
    };
    
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi_handle);
    if (ret != ESP_OK) {
        OSAL_LOGE("Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ret;
    }
    
    s_initialized = true;
    OSAL_LOGI("=== SPI MASTER Initialized ===");
    OSAL_LOGI("MOSI=%d, MISO=%d, SCK=%d, CS=%d, Speed=%dHz",
             SPI_COMM_MOSI_PIN, SPI_COMM_MISO_PIN, SPI_COMM_SCK_PIN,
             SPI_COMM_CS_PIN, SPI_COMM_CLOCK_SPEED_HZ);
    
    return ESP_OK;
}

esp_err_t spi_master_deinit(void) {
    if (!s_initialized) return ESP_OK;
    
    spi_bus_remove_device(s_spi_handle);
    spi_bus_free(SPI2_HOST);
    s_initialized = false;
    
    return ESP_OK;
}

void spi_master_set_data_callback(spi_master_data_callback_t callback) {
    s_data_callback = callback;
}

esp_err_t spi_master_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    
    esp_err_t ret = spi_device_transmit(s_spi_handle, &trans);
    if (ret == ESP_OK) {
        s_transactions++;
    } else {
        s_errors++;
    }
    
    return ret;
}

esp_err_t spi_master_request_data(void) {
    OSAL_LOGI("Requesting sensor data from slave");
    
    // Build request frame
    uint8_t tx_buf[SPI_COMM_MAX_PACKET_SIZE + 6] = {0};
    uint8_t rx_buf[SPI_COMM_MAX_PACKET_SIZE + 6] = {0};
    
    tx_buf[0] = SPI_COMM_START_MARKER;
    tx_buf[1] = SPI_FRAME_TYPE_REQUEST;
    tx_buf[2] = 0;  // Length high
    tx_buf[3] = 0;  // Length low (no payload for request)
    
    // Perform transaction - slave responds with sensor data
    esp_err_t ret = spi_master_transfer(tx_buf, rx_buf, sizeof(tx_buf));
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Parse response
    if (rx_buf[0] == SPI_COMM_START_MARKER && rx_buf[1] == SPI_FRAME_TYPE_SENSOR_DATA) {
        uint16_t payload_len = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
        
        spi_sensor_packet_t packet;
        if (deserialize_sensor_packet(&rx_buf[4], payload_len, &packet)) {
            store_sensor_values(&packet);
            if (s_data_callback) {
                s_data_callback(&packet);
            }
        }
    }
    
    return ESP_OK;
}

esp_err_t spi_master_set_interval(uint32_t interval_ms) {
    OSAL_LOGI("Setting slave interval to %lu ms", interval_ms);
    
    uint8_t tx_buf[16] = {0};
    uint8_t rx_buf[16] = {0};
    
    tx_buf[0] = SPI_COMM_START_MARKER;
    tx_buf[1] = SPI_FRAME_TYPE_CONFIG;
    tx_buf[2] = 0;
    tx_buf[3] = 10;  // Payload length
    tx_buf[4] = SPI_CONFIG_CMD_SET_INTERVAL;
    tx_buf[5] = 0xFF;  // All sensors
    tx_buf[6] = (interval_ms >> 24) & 0xFF;
    tx_buf[7] = (interval_ms >> 16) & 0xFF;
    tx_buf[8] = (interval_ms >> 8) & 0xFF;
    tx_buf[9] = interval_ms & 0xFF;
    
    return spi_master_transfer(tx_buf, rx_buf, sizeof(tx_buf));
}

esp_err_t spi_master_enable_sensor(uint8_t sensor_id, bool enable) {
    uint8_t tx_buf[16] = {0};
    uint8_t rx_buf[16] = {0};
    
    tx_buf[0] = SPI_COMM_START_MARKER;
    tx_buf[1] = SPI_FRAME_TYPE_CONFIG;
    tx_buf[2] = 0;
    tx_buf[3] = 10;
    tx_buf[4] = SPI_CONFIG_CMD_ENABLE_SENSOR;
    tx_buf[5] = sensor_id;
    tx_buf[6] = 0;
    tx_buf[7] = 0;
    tx_buf[8] = 0;
    tx_buf[9] = enable ? 1 : 0;
    
    return spi_master_transfer(tx_buf, rx_buf, sizeof(tx_buf));
}

esp_err_t spi_master_reset_slave(void) {
    uint8_t tx_buf[16] = {0};
    uint8_t rx_buf[16] = {0};
    
    tx_buf[0] = SPI_COMM_START_MARKER;
    tx_buf[1] = SPI_FRAME_TYPE_CONFIG;
    tx_buf[2] = 0;
    tx_buf[3] = 10;
    tx_buf[4] = SPI_CONFIG_CMD_RESET;
    
    return spi_master_transfer(tx_buf, rx_buf, sizeof(tx_buf));
}

/* Data access functions */
float spi_master_get_joystick_up(void) {
    return s_joystick_up;
}
float spi_master_get_joystick_down(void) {
    return s_joystick_down;
}
float spi_master_get_joystick_left(void) {
    return s_joystick_left;
}
float spi_master_get_joystick_right(void) {
    return s_joystick_right;
}
float spi_master_get_joystick_center_press(void) {
    return s_joystick_center_press;
}
float spi_master_get_boot_button(void) {
    return s_boot_button;
}
float spi_master_get_aht10_temperature_humidity_sensor(void) {
    return s_aht10_temperature_humidity_sensor;
}

float spi_master_get_sensor_value(uint8_t sensor_id) {
    for (int i = 0; i < MAX_SENSOR_STORAGE; i++) {
        if (s_sensor_storage[i].valid && s_sensor_storage[i].sensor_id == sensor_id) {
            return s_sensor_storage[i].value;
        }
    }
    return NAN;
}

bool spi_master_is_data_fresh(uint32_t timeout_ms) {
    if (s_last_update_ms == 0) return false;
    uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    return (now - s_last_update_ms) < timeout_ms;
}

uint32_t spi_master_get_data_age(void) {
    if (s_last_update_ms == 0) return UINT32_MAX;
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS) - s_last_update_ms;
}

void spi_master_get_stats(uint32_t *transactions, uint32_t *errors) {
    if (transactions) *transactions = s_transactions;
    if (errors) *errors = s_errors;
}