/**
 * @file periphery_state.h
 * @brief Enhanced IO State Manager

 * 
 * Manages all IO states with:
 * - Central state storage with timestamps
 * - Change detection and callbacks
 * - Thread-safe access
 * - Integration with Pub/Sub middleware (auto-publish on change)
 * 
 * Usage:
 *   // Initialize
 *   periphery_state_init();
 *   
 *   // Register signals (usually auto-generated)
 *   periphery_state_register("temp_sensor/temperature", PERIPHERY_TYPE_FLOAT, PERIPHERY_DIR_OUTPUT, "temp_sensor");
 *   
 *   // Set/get values
 *   periphery_state_set_float("temp_sensor/temperature", 25.5f);
 *   float temp = periphery_state_get_float("temp_sensor/temperature");
 *   
 *   // Register change callback
 *   periphery_state_on_change("temp_sensor/temperature", on_temp_change, NULL);
 *   
 *   // Enable auto-publish to middleware
 *   periphery_state_enable_autopublish("temp_sensor/temperature", "sensors/temperature");
 */

#ifndef PERIPHERY_STATE_H
#define PERIPHERY_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define PERIPHERY_STATE_MAX_SIGNALS      32
#define PERIPHERY_STATE_MAX_NAME_LEN     64
#define PERIPHERY_STATE_MAX_CALLBACKS    8
#define PERIPHERY_STATE_MAX_DATA_SIZE    64

/* ============================================================================
 * Types
 * ============================================================================ */

/**
 * @brief Signal data type
 */
typedef enum {
    PERIPHERY_TYPE_BOOL = 0,
    PERIPHERY_TYPE_INT8,
    PERIPHERY_TYPE_UINT8,
    PERIPHERY_TYPE_INT16,
    PERIPHERY_TYPE_UINT16,
    PERIPHERY_TYPE_INT32,
    PERIPHERY_TYPE_UINT32,
    PERIPHERY_TYPE_INT64,
    PERIPHERY_TYPE_UINT64,
    PERIPHERY_TYPE_FLOAT,
    PERIPHERY_TYPE_DOUBLE,
    PERIPHERY_TYPE_STRING,
    PERIPHERY_TYPE_BYTES,
    PERIPHERY_TYPE_STRUCT
} periphery_type_t;

/**
 * @brief Signal direction
 */
typedef enum {
    PERIPHERY_DIR_INPUT = 0,           /**< Writable (actuator commands) */
    PERIPHERY_DIR_OUTPUT,              /**< Readable (sensor data) */
    PERIPHERY_DIR_BIDIRECTIONAL        /**< Both read and write */
} periphery_direction_t;

/**
 * @brief Signal value union
 */
typedef union {
    bool b;
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    float f;
    double d;
    char str[PERIPHERY_STATE_MAX_DATA_SIZE];
    uint8_t bytes[PERIPHERY_STATE_MAX_DATA_SIZE];
} periphery_value_t;

/**
 * @brief Signal state with metadata
 */
typedef struct {
    periphery_value_t value;           /**< Current value */
    periphery_value_t prev_value;      /**< Previous value (for change detection) */
    uint64_t timestamp_us;      /**< Last update timestamp */
    uint64_t prev_timestamp_us; /**< Previous update timestamp */
    uint32_t update_count;      /**< Number of updates */
    bool valid;                 /**< Value has been set at least once */
    bool changed;               /**< Value changed since last check */
} periphery_state_t;

/**
 * @brief Change callback function type
 * @param signal_name Name of the signal that changed
 * @param new_value Pointer to new value
 * @param old_value Pointer to old value
 * @param user_data User context
 */
typedef void (*periphery_change_callback_t)(
    const char* signal_name,
    const periphery_value_t* new_value,
    const periphery_value_t* old_value,
    void* user_data
);

/**
 * @brief Threshold callback function type
 * @param signal_name Name of the signal
 * @param value Current value
 * @param threshold Threshold that was crossed
 * @param rising true if value crossed threshold going up
 * @param user_data User context
 */
typedef void (*periphery_threshold_callback_t)(
    const char* signal_name,
    const periphery_value_t* value,
    const periphery_value_t* threshold,
    bool rising,
    void* user_data
);

/**
 * @brief Opaque signal handle
 */
typedef struct periphery_signal_s* periphery_signal_handle_t;

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * @brief Initialize the IO state manager
 * @return 0 on success
 */
int periphery_state_init(void);

/**
 * @brief Shutdown the IO state manager
 */
void periphery_state_shutdown(void);

/**
 * @brief Check if initialized
 * @return true if initialized
 */
bool periphery_state_is_initialized(void);

/* ============================================================================
 * Signal Registration
 * ============================================================================ */

/**
 * @brief Register a new signal
 * @param name Signal name (e.g., "sensor/temperature")
 * @param type Data type
 * @param direction I/O direction
 * @param component Component that owns this signal
 * @return Signal handle or NULL on error
 */
periphery_signal_handle_t periphery_state_register(
    const char* name,
    periphery_type_t type,
    periphery_direction_t direction,
    const char* component
);

/**
 * @brief Unregister a signal
 * @param name Signal name
 */
void periphery_state_unregister(const char* name);

/**
 * @brief Find a signal by name
 * @param name Signal name
 * @return Signal handle or NULL if not found
 */
periphery_signal_handle_t periphery_state_find(const char* name);

/**
 * @brief Get signal count
 * @return Number of registered signals
 */
int periphery_state_get_count(void);

/**
 * @brief Get signal by index
 * @param index Signal index (0 to count-1)
 * @return Signal handle or NULL
 */
periphery_signal_handle_t periphery_state_get_by_index(int index);

/**
 * @brief Get signal name
 * @param handle Signal handle
 * @return Signal name or NULL
 */
const char* periphery_state_get_name(periphery_signal_handle_t handle);

/**
 * @brief Get signal type
 * @param handle Signal handle
 * @return Signal type
 */
periphery_type_t periphery_state_get_type(periphery_signal_handle_t handle);

/**
 * @brief Get signal direction
 * @param handle Signal handle
 * @return Signal direction
 */
periphery_direction_t periphery_state_get_direction(periphery_signal_handle_t handle);

/* ============================================================================
 * Value Access - Generic
 * ============================================================================ */

/**
 * @brief Set signal value (generic)
 * @param name Signal name
 * @param value Pointer to value
 * @param size Size of value in bytes
 * @return 0 on success
 */
int periphery_state_set(const char* name, const void* value, size_t size);

/**
 * @brief Get signal value (generic)
 * @param name Signal name
 * @param value Buffer to store value
 * @param size Size of buffer
 * @return 0 on success
 */
int periphery_state_get(const char* name, void* value, size_t size);

/**
 * @brief Get full signal state
 * @param name Signal name
 * @param state Buffer to store state
 * @return 0 on success
 */
int periphery_state_get_state(const char* name, periphery_state_t* state);

/* ============================================================================
 * Value Access - Typed Convenience Functions
 * ============================================================================ */

int periphery_state_set_bool(const char* name, bool value);
bool periphery_state_get_bool(const char* name);

int periphery_state_set_int(const char* name, int32_t value);
int32_t periphery_state_get_int(const char* name);

int periphery_state_set_uint(const char* name, uint32_t value);
uint32_t periphery_state_get_uint(const char* name);

int periphery_state_set_float(const char* name, float value);
float periphery_state_get_float(const char* name);

int periphery_state_set_double(const char* name, double value);
double periphery_state_get_double(const char* name);

int periphery_state_set_string(const char* name, const char* value);
const char* periphery_state_get_string(const char* name);

/* ============================================================================
 * Change Detection & Callbacks
 * ============================================================================ */

/**
 * @brief Register a change callback
 * @param name Signal name (or "*" for all signals)
 * @param callback Callback function
 * @param user_data User context
 * @return 0 on success
 */
int periphery_state_on_change(const char* name, periphery_change_callback_t callback, void* user_data);

/**
 * @brief Remove a change callback
 * @param name Signal name
 * @param callback Callback to remove
 */
void periphery_state_remove_callback(const char* name, periphery_change_callback_t callback);

/**
 * @brief Register a threshold callback (for numeric signals)
 * @param name Signal name
 * @param threshold Threshold value
 * @param callback Callback function
 * @param user_data User context
 * @return 0 on success
 */
int periphery_state_on_threshold(
    const char* name,
    periphery_value_t threshold,
    periphery_threshold_callback_t callback,
    void* user_data
);

/**
 * @brief Check if signal value changed since last check
 * @param name Signal name
 * @return true if changed
 */
bool periphery_state_has_changed(const char* name);

/**
 * @brief Clear changed flag for a signal
 * @param name Signal name
 */
void periphery_state_clear_changed(const char* name);

/**
 * @brief Get time since last update
 * @param name Signal name
 * @return Microseconds since last update, or UINT64_MAX if never updated
 */
uint64_t periphery_state_get_age_us(const char* name);

/* ============================================================================
 * Middleware Integration
 * ============================================================================ */

/**
 * @brief Enable auto-publish to middleware when signal changes
 * @param signal_name Signal name
 * @param topic_name Middleware topic name
 * @return 0 on success
 */
int periphery_state_enable_autopublish(const char* signal_name, const char* topic_name);

/**
 * @brief Disable auto-publish for a signal
 * @param signal_name Signal name
 */
void periphery_state_disable_autopublish(const char* signal_name);

/**
 * @brief Enable auto-subscribe from middleware
 * @param signal_name Signal name
 * @param topic_name Middleware topic name
 * @return 0 on success
 */
int periphery_state_enable_autosubscribe(const char* signal_name, const char* topic_name);

/**
 * @brief Disable auto-subscribe for a signal
 * @param signal_name Signal name
 */
void periphery_state_disable_autosubscribe(const char* signal_name);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Process pending callbacks (call from main loop if using polling mode)
 */
void periphery_state_process(void);

/**
 * @brief Dump all signal states (for debugging)
 */
void periphery_state_dump(void);

/**
 * @brief Get memory usage
 * @return Bytes used by IO state manager
 */
size_t periphery_state_get_memory_usage(void);

#ifdef __cplusplus
}
#endif

#endif /* PERIPHERY_STATE_H */