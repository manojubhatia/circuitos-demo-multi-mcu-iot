/**
 * @file msl.h
 * @brief Message Subscription Layer - Unified API
 *
 * Provides protocol-agnostic subscribe/publish/lifecycle interface.
 * Protocol-specific backends register themselves during initialization.
 *
 * Auto-generated for project: theSpeaker (3 boards)
 */

#ifndef MSL_H
#define MSL_H

#include "msl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the MSL framework.
 * Must be called before any subscribe/publish calls.
 * Protocol backends are registered after this call.
 */
void msl_init(void);

/**
 * Subscribe to messages matching a descriptor.
 *
 * @param desc     Protocol-specific descriptor (CAN ID, MQTT topic, etc.)
 * @param handler  Callback invoked when matching message arrives
 * @param user_ctx User context passed to handler (can be NULL)
 * @return         Subscription handle, or MSL_INVALID_HANDLE on failure
 */
msl_sub_handle_t msl_subscribe(const msl_descriptor_t *desc, msl_handler_t handler, void *user_ctx);

/**
 * Unsubscribe a previously registered subscription.
 *
 * @param handle  Handle returned by msl_subscribe
 * @return        true if successfully unsubscribed
 */
bool msl_unsubscribe(msl_sub_handle_t handle);

/**
 * Publish a message through the appropriate protocol backend.
 *
 * @param desc    Descriptor identifying target (CAN ID, MQTT topic, etc.)
 * @param data    Payload data
 * @param length  Payload length in bytes
 * @return        0 on success, negative error code on failure
 */
int msl_publish(const msl_descriptor_t *desc, const uint8_t *data, size_t length);

/**
 * Get the current lifecycle state of a protocol backend.
 *
 * @param protocol  Protocol identifier
 * @return          Current state (IDLE, BRINGING_UP, OPERATIONAL, etc.)
 */
msl_state_t msl_get_state(msl_protocol_t protocol);

/**
 * Register a callback for protocol error events.
 *
 * @param protocol  Protocol to monitor (MSL_PROTO_COUNT = all protocols)
 * @param handler   Error callback
 */
void msl_on_error(msl_protocol_t protocol, msl_error_handler_t handler);

/**
 * Register a callback for lifecycle state transitions.
 *
 * @param protocol  Protocol to monitor (MSL_PROTO_COUNT = all protocols)
 * @param handler   State change callback
 */
void msl_on_state_change(msl_protocol_t protocol, msl_state_handler_t handler);

/**
 * Process pending messages (call from main loop or dedicated task).
 * Dispatches received messages to registered handlers.
 * For RTOS systems, this runs in the MSL task context.
 */
void msl_process(void);

/**
 * Shut down the MSL framework and all registered backends.
 */
void msl_shutdown(void);

/* ─── Protocol Backend Registration API ─── */

/**
 * Backend operations structure - each protocol implements these.
 */
typedef struct {
    msl_protocol_t protocol;
    const char *name;

    int  (*init)(void);
    void (*deinit)(void);
    int  (*subscribe)(const msl_descriptor_t *desc, uint16_t sub_index);
    int  (*unsubscribe)(uint16_t sub_index);
    int  (*publish)(const msl_descriptor_t *desc, const uint8_t *data, size_t len);
    void (*process)(void);
    msl_state_t (*get_state)(void);
} msl_backend_ops_t;

/**
 * Register a protocol backend with the MSL core.
 *
 * @param ops  Pointer to backend operations (must have static lifetime)
 * @return     0 on success, -1 if max backends reached
 */
int msl_register_backend(const msl_backend_ops_t *ops);

/**
 * Get the string name of a protocol.
 */
const char *msl_protocol_name(msl_protocol_t protocol);

/**
 * Get the string name of a state.
 */
const char *msl_state_name(msl_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* MSL_H */