/**
 * @file component_hooks.h
 * @brief Component interaction hooks for theSpeaker (3 boards)
 * @note Generated file - regenerated when components change.
 *       User implementations in user/ directory are preserved.
 */

#ifndef COMPONENT_HOOKS_H
#define COMPONENT_HOOKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Component Hook Function Types
 * ═══════════════════════════════════════════════════════════════════════════ */

// Sensor hooks
typedef void (*sensor_data_hook_t)(uint8_t sensor_id, float value);
typedef void (*sensor_raw_hook_t)(uint8_t sensor_id, const uint8_t* data, size_t len);
typedef bool (*sensor_filter_hook_t)(uint8_t sensor_id, float* value); // Return false to discard

// Actuator hooks
typedef void (*actuator_pre_hook_t)(uint8_t actuator_id, float* value); // Can modify value
typedef void (*actuator_post_hook_t)(uint8_t actuator_id, float value);

// Input hooks
typedef void (*button_hook_t)(uint8_t button_id, bool pressed, uint32_t duration_ms);
typedef void (*touch_hook_t)(uint16_t x, uint16_t y, uint8_t gesture);
typedef void (*encoder_hook_t)(uint8_t encoder_id, int32_t position, int8_t direction);

// Display hooks
typedef void (*display_pre_hook_t)(uint8_t display_id);
typedef void (*display_post_hook_t)(uint8_t display_id);

// Communication hooks
typedef void (*comm_receive_hook_t)(uint8_t channel_id, const uint8_t* data, size_t len);
typedef bool (*comm_send_hook_t)(uint8_t channel_id, uint8_t* data, size_t* len); // Return false to cancel

/* ═══════════════════════════════════════════════════════════════════════════
 * Component Hooks Structure
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    // Sensor hooks
    sensor_data_hook_t on_sensor_data;       // Called when sensor data available
    sensor_raw_hook_t on_sensor_raw;         // Called with raw sensor data
    sensor_filter_hook_t on_sensor_filter;   // Called to filter sensor data
    
    // Actuator hooks
    actuator_pre_hook_t on_actuator_pre;     // Called before actuator change (can modify)
    actuator_post_hook_t on_actuator_post;   // Called after actuator change
    
    // Input hooks
    button_hook_t on_button_event;           // Called on button press/release
    touch_hook_t on_touch_event;             // Called on touch event
    encoder_hook_t on_encoder_event;         // Called on encoder rotation
    
    // Display hooks
    display_pre_hook_t on_display_pre;       // Called before display update
    display_post_hook_t on_display_post;     // Called after display update
    
    // Communication hooks
    comm_receive_hook_t on_comm_receive;     // Called on data received
    comm_send_hook_t on_comm_send;           // Called before sending (can modify/cancel)
    
} component_hooks_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Global Component Hooks Instance
 * ═══════════════════════════════════════════════════════════════════════════ */

extern component_hooks_t g_component_hooks;

/* ═══════════════════════════════════════════════════════════════════════════
 * Component IDs - Generated from circuit design
 * ═══════════════════════════════════════════════════════════════════════════ */

// Sensor IDs
enum SensorId {
    SENSOR_UNKNOWN = 0,
    SENSOR_INMP441_I2S_MEMS_MICROPHONE = 1,
    SENSOR_COUNT
};

// Actuator IDs
enum ActuatorId {
    ACTUATOR_UNKNOWN = 0,
    ACTUATOR_COUNT
};

// Button IDs
enum ButtonId {
    BUTTON_UNKNOWN = 0,
    BUTTON_COUNT
};

// Display IDs
enum DisplayId {
    DISPLAY_UNKNOWN = 0,
    DISPLAY_COUNT
};

// Communication Channel IDs
enum CommChannelId {
    COMM_UNKNOWN = 0,
    COMM_COUNT
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Hook Invocation Macros
 * ═══════════════════════════════════════════════════════════════════════════ */

// Sensor hooks
#define CALL_SENSOR_DATA_HOOK(id, val) \
    do { if (g_component_hooks.on_sensor_data) g_component_hooks.on_sensor_data((id), (val)); } while(0)

#define CALL_SENSOR_RAW_HOOK(id, data, len) \
    do { if (g_component_hooks.on_sensor_raw) g_component_hooks.on_sensor_raw((id), (data), (len)); } while(0)

#define CALL_SENSOR_FILTER_HOOK(id, val_ptr) \
    (g_component_hooks.on_sensor_filter ? g_component_hooks.on_sensor_filter((id), (val_ptr)) : true)

// Actuator hooks
#define CALL_ACTUATOR_PRE_HOOK(id, val_ptr) \
    do { if (g_component_hooks.on_actuator_pre) g_component_hooks.on_actuator_pre((id), (val_ptr)); } while(0)

#define CALL_ACTUATOR_POST_HOOK(id, val) \
    do { if (g_component_hooks.on_actuator_post) g_component_hooks.on_actuator_post((id), (val)); } while(0)

// Input hooks
#define CALL_BUTTON_HOOK(id, pressed, duration) \
    do { if (g_component_hooks.on_button_event) g_component_hooks.on_button_event((id), (pressed), (duration)); } while(0)

#define CALL_TOUCH_HOOK(x, y, gesture) \
    do { if (g_component_hooks.on_touch_event) g_component_hooks.on_touch_event((x), (y), (gesture)); } while(0)

#define CALL_ENCODER_HOOK(id, pos, dir) \
    do { if (g_component_hooks.on_encoder_event) g_component_hooks.on_encoder_event((id), (pos), (dir)); } while(0)

// Display hooks
#define CALL_DISPLAY_PRE_HOOK(id) \
    do { if (g_component_hooks.on_display_pre) g_component_hooks.on_display_pre(id); } while(0)

#define CALL_DISPLAY_POST_HOOK(id) \
    do { if (g_component_hooks.on_display_post) g_component_hooks.on_display_post(id); } while(0)

// Communication hooks
#define CALL_COMM_RECEIVE_HOOK(ch, data, len) \
    do { if (g_component_hooks.on_comm_receive) g_component_hooks.on_comm_receive((ch), (data), (len)); } while(0)

#define CALL_COMM_SEND_HOOK(ch, data, len_ptr) \
    (g_component_hooks.on_comm_send ? g_component_hooks.on_comm_send((ch), (data), (len_ptr)) : true)

/* ═══════════════════════════════════════════════════════════════════════════
 * Hook Registration Helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

static inline void component_hooks_init(void) {
    memset(&g_component_hooks, 0, sizeof(g_component_hooks));
}

static inline void register_sensor_data_hook(sensor_data_hook_t hook) {
    g_component_hooks.on_sensor_data = hook;
}

static inline void register_button_hook(button_hook_t hook) {
    g_component_hooks.on_button_event = hook;
}

static inline void register_actuator_pre_hook(actuator_pre_hook_t hook) {
    g_component_hooks.on_actuator_pre = hook;
}

#ifdef __cplusplus
}
#endif

#endif // COMPONENT_HOOKS_H