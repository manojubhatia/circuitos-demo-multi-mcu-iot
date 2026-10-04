/**
 * Periphery Handler - Central Peripheral IO Management System
 * Auto-generated from Jinja template
 * 
 * This module manages all periphery channels (digital, analog, sensors, communication)
 * with frequency-based update scheduling and event-driven callbacks.
 * 
 * Generated for: thespeaker_3_boards
 */

#ifndef PERIPHERY_HANDLER_H
#define PERIPHERY_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "osal/osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Configuration - Generated from project settings
//=============================================================================

#define PERIPHERY_HANDLER_MAX_CHANNELS     32
#define PERIPHERY_HANDLER_TASK_PRIORITY    5
#define PERIPHERY_HANDLER_STACK_SIZE       4096
#define PERIPHERY_HANDLER_MIN_INTERVAL_MS  10

//=============================================================================
// Periphery Channel Types
//=============================================================================

typedef enum {
    PERIPHERY_TYPE_DIGITAL_INPUT,      // Digital input (button, PIR, etc.)
    PERIPHERY_TYPE_DIGITAL_OUTPUT,     // Digital output (LED, relay, etc.)
    PERIPHERY_TYPE_ANALOG_INPUT,       // ADC input
    PERIPHERY_TYPE_ANALOG_OUTPUT,      // DAC/PWM output
    PERIPHERY_TYPE_I2C_SENSOR,         // I2C sensor (BMP280, MPU6050, etc.)
    PERIPHERY_TYPE_SPI_SENSOR,         // SPI sensor
    PERIPHERY_TYPE_UART_DEVICE,        // UART device
    PERIPHERY_TYPE_ONEWIRE_SENSOR,     // 1-Wire sensor (DHT, DS18B20)
    PERIPHERY_TYPE_CUSTOM,             // Custom periphery type
} periphery_type_t;

typedef enum {
    PERIPHERY_DATA_BOOL,
    PERIPHERY_DATA_INT8,
    PERIPHERY_DATA_INT16,
    PERIPHERY_DATA_INT32,
    PERIPHERY_DATA_UINT8,
    PERIPHERY_DATA_UINT16,
    PERIPHERY_DATA_UINT32,
    PERIPHERY_DATA_FLOAT,
    PERIPHERY_DATA_DOUBLE,
    PERIPHERY_DATA_STRING,
    PERIPHERY_DATA_BLOB,
} periphery_data_type_t;

typedef union {
    bool b;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    float f;
    double d;
    struct {
        void *data;
        size_t len;
    } blob;
} periphery_value_t;

typedef struct {
    const char *name;
    periphery_type_t type;
    periphery_data_type_t data_type;
    periphery_value_t value;
    uint32_t timestamp_ms;
    bool valid;
    const char *unit;
} periphery_channel_data_t;

//=============================================================================
// Callback Types
//=============================================================================

typedef esp_err_t (*periphery_read_func_t)(uint8_t channel_id, periphery_value_t *value);
typedef esp_err_t (*periphery_write_func_t)(uint8_t channel_id, periphery_value_t value);
typedef void (*periphery_callback_t)(uint8_t channel_id, const periphery_channel_data_t *data);

//=============================================================================
// Channel Configuration
//=============================================================================

typedef struct {
    const char *name;
    periphery_type_t type;
    periphery_data_type_t data_type;
    const char *unit;
    
    uint32_t update_interval_ms;
    bool update_on_change;
    float change_threshold;
    
    periphery_read_func_t read_func;
    periphery_write_func_t write_func;
    periphery_callback_t callback;
    
    union {
        struct {
            uint8_t pin;
            bool active_low;
            bool pull_up;
            bool pull_down;
        } digital;
        
        struct {
            uint8_t pin;
            uint8_t attenuation;
            uint8_t samples;
        } analog;
        
        struct {
            uint8_t i2c_port;
            uint8_t i2c_addr;
        } i2c;
        
        struct {
            uint8_t spi_host;
            uint8_t cs_pin;
        } spi;
        
        struct {
            uint8_t uart_num;
            uint32_t baud_rate;
        } uart;
        
        struct {
            uint8_t data_pin;
        } onewire;
    } hw;
} periphery_channel_config_t;

//=============================================================================
// Periphery Handler API
//=============================================================================

esp_err_t periphery_handler_init(void);
void periphery_handler_deinit(void);

esp_err_t periphery_handler_register_channel(const periphery_channel_config_t *config, uint8_t *channel_id);
esp_err_t periphery_handler_unregister_channel(uint8_t channel_id);

int periphery_handler_get_channel_by_name(const char *name);
esp_err_t periphery_handler_read(uint8_t channel_id, periphery_channel_data_t *data);
esp_err_t periphery_handler_read_by_name(const char *name, periphery_channel_data_t *data);
esp_err_t periphery_handler_write(uint8_t channel_id, periphery_value_t value);
esp_err_t periphery_handler_write_by_name(const char *name, periphery_value_t value);
esp_err_t periphery_handler_force_update(uint8_t channel_id);

esp_err_t periphery_handler_start(uint8_t task_priority, uint32_t stack_size);
void periphery_handler_stop(void);

void periphery_handler_set_global_callback(periphery_callback_t callback);
int periphery_handler_get_all_channels(periphery_channel_data_t *data, int max_channels);
int periphery_handler_get_channel_count(void);

//=============================================================================
// Convenience Macros
//=============================================================================

#define PERIPHERY_DIGITAL_INPUT(name, pin, pull_up) \
    { .name = name, .type = PERIPHERY_TYPE_DIGITAL_INPUT, .data_type = PERIPHERY_DATA_BOOL, \
      .update_interval_ms = 50, .update_on_change = true, \
      .hw.digital = { .pin = pin, .pull_up = pull_up } }

#define PERIPHERY_DIGITAL_OUTPUT(name, pin, active_low) \
    { .name = name, .type = PERIPHERY_TYPE_DIGITAL_OUTPUT, .data_type = PERIPHERY_DATA_BOOL, \
      .update_interval_ms = 0, .hw.digital = { .pin = pin, .active_low = active_low } }

#define PERIPHERY_ANALOG_INPUT(name, pin, interval_ms) \
    { .name = name, .type = PERIPHERY_TYPE_ANALOG_INPUT, .data_type = PERIPHERY_DATA_FLOAT, \
      .update_interval_ms = interval_ms, .hw.analog = { .pin = pin, .attenuation = 3, .samples = 4 } }

#define PERIPHERY_I2C_SENSOR(name, port, addr, interval_ms) \
    { .name = name, .type = PERIPHERY_TYPE_I2C_SENSOR, .data_type = PERIPHERY_DATA_FLOAT, \
      .update_interval_ms = interval_ms, .hw.i2c = { .i2c_port = port, .i2c_addr = addr } }

#ifdef __cplusplus
}
#endif

#endif // PERIPHERY_HANDLER_H
