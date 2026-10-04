/**
 * @file protocol_handler.h
 * @brief Protocol Handler for Inter-MCU Communication
 * 
 * This module handles the protocol layer for data exchange between MCUs.
 * It sits between the IO Handler (sensor data) and the Transport Layer (physical).
 * 
 * Architecture:
 *   Sender MCU:   IO Handler → Protocol Handler (send) → Transport
 *   Receiver MCU: Transport → Protocol Handler (receive) → Application
 *   Relay MCU:    Transport → Protocol Handler (receive+forward) → Transport
 * 
 * Role: both
 * Transport: spi
 * 
 * Auto-generated
 */

#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "osal/osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Protocol Constants
//=============================================================================

#define PROTOCOL_MAGIC          0x50524F54  // "PROT"
#define PROTOCOL_VERSION        1
#define PROTOCOL_MAX_PAYLOAD    128
#define PROTOCOL_MAX_SIGNALS    16

//=============================================================================
// Message Types
//=============================================================================

typedef enum {
    MSG_TYPE_HEARTBEAT      = 0x00,
    MSG_TYPE_ACK            = 0x01,
    MSG_TYPE_SENSOR_DATA    = 0x10,
    MSG_TYPE_COMMAND        = 0x20,
    MSG_TYPE_STATUS         = 0x30,
    MSG_TYPE_RELAY          = 0x40,  // For relay/forward messages
} protocol_msg_type_t;

//=============================================================================
// Data Types for Signals
//=============================================================================

typedef enum {
    SIGNAL_TYPE_FLOAT,
    SIGNAL_TYPE_INT32,
    SIGNAL_TYPE_BOOL,
    SIGNAL_TYPE_VECTOR3F,
    SIGNAL_TYPE_BYTES
} signal_type_t;

typedef union {
    float f;
    int32_t i;
    bool b;
    struct { float x, y, z; } vec3;
    uint8_t bytes[16];
} signal_value_t;

//=============================================================================
// Message Header
//=============================================================================

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t msg_type;
    uint16_t sequence;
    uint32_t timestamp_ms;
    uint16_t payload_len;
    uint16_t checksum;
} __attribute__((packed)) protocol_header_t;

//=============================================================================
// Signal Definition
//=============================================================================

typedef struct {
    char name[16];          // Signal name (e.g., "switch_state")
    signal_type_t type;     // Data type
    signal_value_t value;   // Current value
    uint32_t last_update;   // Last update timestamp
} protocol_signal_t;

//=============================================================================
// Sensor Data Message
//=============================================================================

typedef struct {
    protocol_header_t header;
    uint8_t signal_count;
    // Followed by signal_count * protocol_signal_t
} __attribute__((packed)) sensor_data_msg_t;

//=============================================================================
// Protocol Handler Configuration
//=============================================================================

typedef struct {
    bool is_sender;                     // true = produces signals, false = consumes signals
    bool is_relay;                      // true = relay MCU (receives and forwards)
    uint32_t send_interval_ms;          // Interval for sending data (sender only)
    void (*on_data_received)(const protocol_signal_t *signals, uint8_t count);  // Callback on data reception
    void (*on_relay_forward)(const protocol_signal_t *signals, uint8_t count);  // Callback to forward data (relay only)
} protocol_handler_config_t;

//=============================================================================
// API Functions
//=============================================================================

/**
 * @brief Initialize the protocol handler
 * @param config Configuration
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_init(const protocol_handler_config_t *config);

/**
 * @brief Deinitialize the protocol handler
 */
void protocol_handler_deinit(void);

//=============================================================================
// Sender Functions (for producing signals)
//=============================================================================

/**
 * @brief Register a signal to be sent
 * @param name Signal name
 * @param type Signal data type
 * @return Signal index, or -1 on error
 */
int protocol_handler_register_signal(const char *name, signal_type_t type);

/**
 * @brief Update a signal value (will be sent on next interval)
 * @param index Signal index from register
 * @param value New value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_update_signal(int index, signal_value_t value);

/**
 * @brief Update a signal by name
 * @param name Signal name
 * @param value New value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_update_signal_by_name(const char *name, signal_value_t value);

/**
 * @brief Force immediate send of all signals
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_send_now(void);

/**
 * @brief Send a boolean signal (convenience function)
 * @param name Signal name
 * @param value Boolean value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_send_bool(const char *name, bool value);

/**
 * @brief Send an integer signal (convenience function)
 * @param name Signal name
 * @param value Integer value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_send_int(const char *name, int32_t value);

/**
 * @brief Send a float signal (convenience function)
 * @param name Signal name
 * @param value Float value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_send_float(const char *name, float value);

//=============================================================================
// Receiver Functions (for consuming signals)
//=============================================================================

/**
 * @brief Get the latest value of a signal by name
 * @param name Signal name
 * @param value Output value
 * @return ESP_OK if found, ESP_ERR_NOT_FOUND otherwise
 */
esp_err_t protocol_handler_get_signal(const char *name, signal_value_t *value);

/**
 * @brief Get all received signals
 * @param signals Output array
 * @param max_count Maximum signals to return
 * @return Number of signals copied
 */
int protocol_handler_get_all_signals(protocol_signal_t *signals, int max_count);

/**
 * @brief Check if a signal has been updated recently
 * @param name Signal name
 * @param max_age_ms Maximum age in milliseconds
 * @return true if signal is fresh
 */
bool protocol_handler_is_signal_fresh(const char *name, uint32_t max_age_ms);

/**
 * @brief Get a boolean signal (convenience function)
 * @param name Signal name
 * @param value Output value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_get_bool(const char *name, bool *value);

/**
 * @brief Get an integer signal (convenience function)
 * @param name Signal name
 * @param value Output value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_get_int(const char *name, int32_t *value);

/**
 * @brief Get a float signal (convenience function)
 * @param name Signal name
 * @param value Output value
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_get_float(const char *name, float *value);

/**
 * @brief Start the protocol handler task
 * @return ESP_OK on success
 */
esp_err_t protocol_handler_start(void);

/**
 * @brief Stop the protocol handler task
 */
void protocol_handler_stop(void);

/**
 * @brief Get protocol statistics
 * @param tx_count Transmitted message count
 * @param rx_count Received message count
 * @param error_count Error count
 */
void protocol_handler_get_stats(uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_HANDLER_H
