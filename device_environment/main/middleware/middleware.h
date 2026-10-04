/**
 * @file middleware.h
 * @brief Pub/Sub Middleware - Main API

 * 
 * ROS2-style publish/subscribe middleware for embedded systems.
 * Decouples components through named topics with typed messages.
 * 
 * Usage:
 *   // Initialize middleware
 *   mw_init();
 *   
 *   // Create publisher
 *   mw_publisher_handle_t pub;
 *   mw_create_publisher("sensors/temperature", sizeof(mw_msg_temperature_t), MW_QOS_RELIABLE, &pub);
 *   
 *   // Create subscriber
 *   mw_subscriber_handle_t sub;
 *   mw_create_subscriber("sensors/temperature", on_temperature, NULL, MW_QOS_RELIABLE, &sub);
 *   
 *   // Publish
 *   mw_msg_temperature_t msg = { .temperature_c = 25.5f };
 *   mw_publish(pub, &msg);
 *   
 *   // Process in main loop
 *   while (1) {
 *       mw_spin_once(100);  // Process callbacks, 100ms timeout
 *   }
 */

#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "middleware_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * @brief Initialize the middleware
 * @return 0 on success, negative error code on failure
 */
int mw_init(void);

/**
 * @brief Shutdown the middleware
 */
void mw_shutdown(void);

/**
 * @brief Check if middleware is initialized
 * @return true if initialized
 */
bool mw_is_initialized(void);

/* ============================================================================
 * Topic Management
 * ============================================================================ */

/**
 * @brief Create/register a topic
 * @param name Topic name (e.g., "sensors/temperature")
 * @param msg_size Size of messages on this topic
 * @param qos Quality of service
 * @param handle Output handle
 * @return 0 on success
 */
int mw_create_topic(const char* name, size_t msg_size, mw_qos_t qos, mw_topic_handle_t* handle);

/**
 * @brief Find a topic by name
 * @param name Topic name
 * @return Topic handle or NULL if not found
 */
mw_topic_handle_t mw_find_topic(const char* name);

/**
 * @brief Get topic name
 * @param handle Topic handle
 * @return Topic name or NULL
 */
const char* mw_get_topic_name(mw_topic_handle_t handle);

/**
 * @brief Get number of subscribers on a topic
 * @param handle Topic handle
 * @return Subscriber count
 */
int mw_get_subscriber_count(mw_topic_handle_t handle);

/* ============================================================================
 * Publisher API
 * ============================================================================ */

/**
 * @brief Create a publisher for a topic
 * @param topic_name Topic name
 * @param msg_size Message size
 * @param qos Quality of service
 * @param handle Output publisher handle
 * @return 0 on success
 */
int mw_create_publisher(const char* topic_name, size_t msg_size, mw_qos_t qos, mw_publisher_handle_t* handle);

/**
 * @brief Destroy a publisher
 * @param handle Publisher handle
 */
void mw_destroy_publisher(mw_publisher_handle_t handle);

/**
 * @brief Publish a message
 * @param handle Publisher handle
 * @param msg Pointer to message data
 * @return 0 on success
 */
int mw_publish(mw_publisher_handle_t handle, const void* msg);

/**
 * @brief Publish with header (auto-fills timestamp and sequence)
 * @param handle Publisher handle
 * @param msg Pointer to message (first field must be mw_header_t)
 * @return 0 on success
 */
int mw_publish_with_header(mw_publisher_handle_t handle, void* msg);

/**
 * @brief Publish raw bytes to a topic (auto-creates topic if needed)
 * @param topic_name Topic name
 * @param data Raw byte data
 * @param len Data length
 * @return 0 on success
 */
int mw_publish_raw(const char* topic_name, const uint8_t* data, size_t len);

/* ============================================================================
 * Subscriber API
 * ============================================================================ */

/**
 * @brief Create a subscriber for a topic
 * @param topic_name Topic name
 * @param callback Callback function for received messages
 * @param user_data User context passed to callback
 * @param qos Quality of service
 * @param handle Output subscriber handle
 * @return 0 on success
 */
int mw_create_subscriber(
    const char* topic_name,
    mw_subscriber_callback_t callback,
    void* user_data,
    mw_qos_t qos,
    mw_subscriber_handle_t* handle
);

/**
 * @brief Destroy a subscriber
 * @param handle Subscriber handle
 */
void mw_destroy_subscriber(mw_subscriber_handle_t handle);

/**
 * @brief Set subscriber callback
 * @param handle Subscriber handle
 * @param callback New callback function
 * @param user_data New user context
 */
void mw_set_subscriber_callback(mw_subscriber_handle_t handle, mw_subscriber_callback_t callback, void* user_data);

/* ============================================================================
 * Message Processing
 * ============================================================================ */

/**
 * @brief Process pending messages (call from main loop)
 * @param timeout_ms Maximum time to spend processing (0 = return immediately)
 * @return Number of messages processed
 */
int mw_spin_once(uint32_t timeout_ms);

/**
 * @brief Block and process messages indefinitely
 * @note This function does not return
 */
void mw_spin(void);

/**
 * @brief Check if there are pending messages
 * @return true if messages are pending
 */
bool mw_has_pending_messages(void);

/* ============================================================================
 * Statistics
 * ============================================================================ */

/**
 * @brief Get total number of published messages
 * @return Message count
 */
uint32_t mw_get_publish_count(void);

/**
 * @brief Get total number of delivered messages
 * @return Message count
 */
uint32_t mw_get_deliver_count(void);

/**
 * @brief Get number of dropped messages
 * @return Message count
 */
uint32_t mw_get_drop_count(void);

/**
 * @brief Reset statistics
 */
void mw_reset_stats(void);

#ifdef __cplusplus
}
#endif

/* ============================================================================
 * C++ API
 * ============================================================================ */
#ifdef __cplusplus

#include <functional>
#include <string>

namespace mw {

/**
 * @brief C++ Publisher wrapper
 */
template<typename MsgT>
class Publisher {
public:
    Publisher(const std::string& topic, mw_qos_t qos = MW_QOS_BEST_EFFORT) {
        mw_create_publisher(topic.c_str(), sizeof(MsgT), qos, &_handle);
    }
    
    ~Publisher() {
        if (_handle) mw_destroy_publisher(_handle);
    }
    
    int publish(const MsgT& msg) {
        return mw_publish(_handle, &msg);
    }
    
    int publishWithHeader(MsgT& msg) {
        return mw_publish_with_header(_handle, &msg);
    }
    
    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;
    
private:
    mw_publisher_handle_t _handle = nullptr;
};

/**
 * @brief C++ Subscriber wrapper
 */
template<typename MsgT>
class Subscriber {
public:
    using Callback = std::function<void(const MsgT&)>;
    
    Subscriber(const std::string& topic, Callback cb, mw_qos_t qos = MW_QOS_BEST_EFFORT)
        : _callback(cb) {
        mw_create_subscriber(
            topic.c_str(),
            [](const char*, const void* msg, size_t, void* ctx) {
                auto* self = static_cast<Subscriber*>(ctx);
                if (self->_callback) {
                    self->_callback(*static_cast<const MsgT*>(msg));
                }
            },
            this,
            qos,
            &_handle
        );
    }
    
    ~Subscriber() {
        if (_handle) mw_destroy_subscriber(_handle);
    }
    
    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;
    
private:
    mw_subscriber_handle_t _handle = nullptr;
    Callback _callback;
};

} // namespace mw

#endif // __cplusplus

#endif /* MIDDLEWARE_H */