/**
 * @file middleware.c
 * @brief Pub/Sub Middleware - Implementation

 */

#include "middleware.h"
#include "../osal/osal.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

typedef struct mw_subscriber_s {
    mw_subscriber_callback_t callback;
    void* user_data;
    mw_qos_t qos;
    bool active;
} mw_subscriber_t;

typedef struct mw_topic_s {
    char name[MW_MAX_TOPIC_NAME_LEN];
    size_t msg_size;
    mw_qos_t qos;
    bool active;
    
    /* Subscribers */
    mw_subscriber_t subscribers[MW_MAX_SUBSCRIBERS];
    int subscriber_count;
    
    /* History buffer for TRANSIENT_LOCAL */
    uint8_t* history_buffer;
    int history_head;
    int history_count;
    
    /* Statistics */
    uint32_t publish_count;
    uint32_t deliver_count;
} mw_topic_t;

typedef struct mw_publisher_s {
    mw_topic_handle_t topic;
    uint32_t seq;
    bool active;
} mw_publisher_t;

typedef struct {
    mw_topic_handle_t topic;
    uint8_t data[MW_MAX_MESSAGE_SIZE];
    size_t size;
} mw_pending_msg_t;

/* ============================================================================
 * Static Data
 * ============================================================================ */

static struct {
    bool initialized;
    mw_topic_t topics[MW_MAX_TOPICS];
    int topic_count;
    
    /* Message queue for async delivery */
    osal_queue_handle_t msg_queue;
    osal_mutex_handle_t mutex;
    
    /* Global statistics */
    uint32_t total_publish;
    uint32_t total_deliver;
    uint32_t total_drop;
} s_mw;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static mw_topic_t* find_topic_internal(const char* name) {
    for (int i = 0; i < s_mw.topic_count; i++) {
        if (s_mw.topics[i].active && strcmp(s_mw.topics[i].name, name) == 0) {
            return &s_mw.topics[i];
        }
    }
    return NULL;
}

static mw_topic_t* allocate_topic(void) {
    if (s_mw.topic_count >= MW_MAX_TOPICS) {
        return NULL;
    }
    mw_topic_t* topic = &s_mw.topics[s_mw.topic_count++];
    memset(topic, 0, sizeof(mw_topic_t));
    topic->active = true;
    return topic;
}

static void deliver_to_subscribers(mw_topic_t* topic, const void* msg, size_t size) {
    for (int i = 0; i < MW_MAX_SUBSCRIBERS; i++) {
        mw_subscriber_t* sub = &topic->subscribers[i];
        if (sub->active && sub->callback) {
            sub->callback(topic->name, msg, size, sub->user_data);
            topic->deliver_count++;
            s_mw.total_deliver++;
        }
    }
}

static void store_in_history(mw_topic_t* topic, const void* msg) {
    if (!topic->history_buffer) {
        topic->history_buffer = malloc(MW_HISTORY_DEPTH * topic->msg_size);
        if (!topic->history_buffer) return;
        topic->history_head = 0;
        topic->history_count = 0;
    }
    
    /* Store at head position */
    memcpy(topic->history_buffer + topic->history_head * topic->msg_size, msg, topic->msg_size);
    topic->history_head = (topic->history_head + 1) % MW_HISTORY_DEPTH;
    if (topic->history_count < MW_HISTORY_DEPTH) {
        topic->history_count++;
    }
}

static void deliver_history_to_subscriber(mw_topic_t* topic, mw_subscriber_t* sub) {
    if (!topic->history_buffer || topic->history_count == 0) return;
    
    /* Deliver history from oldest to newest */
    int start = (topic->history_head - topic->history_count + MW_HISTORY_DEPTH) % MW_HISTORY_DEPTH;
    for (int i = 0; i < topic->history_count; i++) {
        int idx = (start + i) % MW_HISTORY_DEPTH;
        const void* msg = topic->history_buffer + idx * topic->msg_size;
        sub->callback(topic->name, msg, topic->msg_size, sub->user_data);
    }
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

int mw_init(void) {
    if (s_mw.initialized) return 0;
    
    memset(&s_mw, 0, sizeof(s_mw));
    
    /* Create message queue for async delivery */
    if (osal_queue_create(&s_mw.msg_queue, sizeof(mw_pending_msg_t), 32) != OSAL_OK) {
        return -1;
    }
    
    /* Create mutex for thread safety */
    if (osal_mutex_create(&s_mw.mutex) != OSAL_OK) {
        osal_queue_delete(s_mw.msg_queue);
        return -1;
    }
    
    s_mw.initialized = true;
    return 0;
}

void mw_shutdown(void) {
    if (!s_mw.initialized) return;
    
    /* Free history buffers */
    for (int i = 0; i < s_mw.topic_count; i++) {
        if (s_mw.topics[i].history_buffer) {
            free(s_mw.topics[i].history_buffer);
        }
    }
    
    osal_queue_delete(s_mw.msg_queue);
    osal_mutex_delete(s_mw.mutex);
    
    memset(&s_mw, 0, sizeof(s_mw));
}

bool mw_is_initialized(void) {
    return s_mw.initialized;
}

/* ============================================================================
 * Topic Management
 * ============================================================================ */

int mw_create_topic(const char* name, size_t msg_size, mw_qos_t qos, mw_topic_handle_t* handle) {
    if (!s_mw.initialized || !name || !handle) return -1;
    
    osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
    
    /* Check if topic already exists */
    mw_topic_t* existing = find_topic_internal(name);
    if (existing) {
        *handle = (mw_topic_handle_t)existing;
        osal_mutex_unlock(s_mw.mutex);
        return 0;
    }
    
    /* Create new topic */
    mw_topic_t* topic = allocate_topic();
    if (!topic) {
        osal_mutex_unlock(s_mw.mutex);
        return -1;
    }
    
    strncpy(topic->name, name, MW_MAX_TOPIC_NAME_LEN - 1);
    topic->msg_size = msg_size;
    topic->qos = qos;
    
    *handle = (mw_topic_handle_t)topic;
    osal_mutex_unlock(s_mw.mutex);
    return 0;
}

mw_topic_handle_t mw_find_topic(const char* name) {
    if (!s_mw.initialized || !name) return NULL;
    
    osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
    mw_topic_t* topic = find_topic_internal(name);
    osal_mutex_unlock(s_mw.mutex);
    
    return (mw_topic_handle_t)topic;
}

const char* mw_get_topic_name(mw_topic_handle_t handle) {
    mw_topic_t* topic = (mw_topic_t*)handle;
    return topic ? topic->name : NULL;
}

int mw_get_subscriber_count(mw_topic_handle_t handle) {
    mw_topic_t* topic = (mw_topic_t*)handle;
    return topic ? topic->subscriber_count : 0;
}

/* ============================================================================
 * Publisher API
 * ============================================================================ */

int mw_create_publisher(const char* topic_name, size_t msg_size, mw_qos_t qos, mw_publisher_handle_t* handle) {
    if (!s_mw.initialized || !topic_name || !handle) return -1;
    
    /* Ensure topic exists */
    mw_topic_handle_t topic_handle;
    if (mw_create_topic(topic_name, msg_size, qos, &topic_handle) != 0) {
        return -1;
    }
    
    mw_publisher_t* pub = malloc(sizeof(mw_publisher_t));
    if (!pub) return -1;
    
    pub->topic = topic_handle;
    pub->seq = 0;
    pub->active = true;
    
    *handle = (mw_publisher_handle_t)pub;
    return 0;
}

void mw_destroy_publisher(mw_publisher_handle_t handle) {
    if (handle) {
        free(handle);
    }
}

int mw_publish(mw_publisher_handle_t handle, const void* msg) {
    if (!handle || !msg) return -1;
    
    mw_publisher_t* pub = (mw_publisher_t*)handle;
    mw_topic_t* topic = (mw_topic_t*)pub->topic;
    
    if (!topic || !pub->active) return -1;
    
    osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
    
    topic->publish_count++;
    s_mw.total_publish++;
    
    /* Store in history if QoS requires it */
    if (topic->qos == MW_QOS_TRANSIENT_LOCAL) {
        store_in_history(topic, msg);
    }
    
    /* Direct delivery to subscribers */
    deliver_to_subscribers(topic, msg, topic->msg_size);
    
    osal_mutex_unlock(s_mw.mutex);
    return 0;
}

int mw_publish_with_header(mw_publisher_handle_t handle, void* msg) {
    if (!handle || !msg) return -1;
    
    mw_publisher_t* pub = (mw_publisher_t*)handle;
    mw_header_t* header = (mw_header_t*)msg;
    
    /* Fill header */
    header->seq = pub->seq++;
    header->timestamp_us = osal_get_time_us();
    
    return mw_publish(handle, msg);
}

int mw_publish_raw(const char* topic_name, const uint8_t* data, size_t len) {
    if (!s_mw.initialized || !topic_name || !data || len == 0) return -1;
    
    osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
    mw_topic_t* topic = find_topic_internal(topic_name);
    if (!topic) {
        osal_mutex_unlock(s_mw.mutex);
        /* Auto-create topic + publisher outside the lock to avoid deadlock */
        mw_topic_handle_t th;
        int rc = mw_create_topic(topic_name, len, MW_QOS_BEST_EFFORT, &th);
        if (rc != 0) return rc;
        
        mw_publisher_handle_t pub;
        rc = mw_create_publisher(topic_name, len, MW_QOS_BEST_EFFORT, &pub);
        if (rc != 0) return rc;
        rc = mw_publish(pub, data);
        mw_destroy_publisher(pub);
        return rc;
    }
    
    /* Deliver to subscribers synchronously — match callback signature:
       void (*)(const char* topic, const void* msg, size_t msg_size, void* user_data) */
    for (int i = 0; i < topic->subscriber_count; i++) {
        mw_subscriber_t* sub = &topic->subscribers[i];
        if (sub->callback) {
            sub->callback(topic_name, (const void*)data, len, sub->user_data);
        }
    }
    osal_mutex_unlock(s_mw.mutex);
    return 0;
}

/* ============================================================================
 * Subscriber API
 * ============================================================================ */

int mw_create_subscriber(
    const char* topic_name,
    mw_subscriber_callback_t callback,
    void* user_data,
    mw_qos_t qos,
    mw_subscriber_handle_t* handle
) {
    if (!s_mw.initialized || !topic_name || !callback || !handle) return -1;
    
    osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
    
    /* Find or create topic */
    mw_topic_t* topic = find_topic_internal(topic_name);
    if (!topic) {
        topic = allocate_topic();
        if (!topic) {
            osal_mutex_unlock(s_mw.mutex);
            return -1;
        }
        strncpy(topic->name, topic_name, MW_MAX_TOPIC_NAME_LEN - 1);
        topic->msg_size = MW_MAX_MESSAGE_SIZE; /* Will be updated by publisher */
        topic->qos = qos;
    }
    
    /* Find free subscriber slot */
    mw_subscriber_t* sub = NULL;
    for (int i = 0; i < MW_MAX_SUBSCRIBERS; i++) {
        if (!topic->subscribers[i].active) {
            sub = &topic->subscribers[i];
            break;
        }
    }
    
    if (!sub) {
        osal_mutex_unlock(s_mw.mutex);
        return -1;
    }
    
    sub->callback = callback;
    sub->user_data = user_data;
    sub->qos = qos;
    sub->active = true;
    topic->subscriber_count++;
    
    /* Deliver history for TRANSIENT_LOCAL */
    if (topic->qos == MW_QOS_TRANSIENT_LOCAL) {
        deliver_history_to_subscriber(topic, sub);
    }
    
    *handle = (mw_subscriber_handle_t)sub;
    osal_mutex_unlock(s_mw.mutex);
    return 0;
}

void mw_destroy_subscriber(mw_subscriber_handle_t handle) {
    if (!handle) return;
    
    osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
    
    mw_subscriber_t* sub = (mw_subscriber_t*)handle;
    sub->active = false;
    sub->callback = NULL;
    
    /* Find and update topic subscriber count */
    for (int i = 0; i < s_mw.topic_count; i++) {
        mw_topic_t* topic = &s_mw.topics[i];
        for (int j = 0; j < MW_MAX_SUBSCRIBERS; j++) {
            if (&topic->subscribers[j] == sub) {
                topic->subscriber_count--;
                break;
            }
        }
    }
    
    osal_mutex_unlock(s_mw.mutex);
}

void mw_set_subscriber_callback(mw_subscriber_handle_t handle, mw_subscriber_callback_t callback, void* user_data) {
    if (!handle) return;
    
    mw_subscriber_t* sub = (mw_subscriber_t*)handle;
    sub->callback = callback;
    sub->user_data = user_data;
}

/* ============================================================================
 * Message Processing
 * ============================================================================ */

int mw_spin_once(uint32_t timeout_ms) {
    if (!s_mw.initialized) return 0;
    
    mw_pending_msg_t msg;
    int processed = 0;
    
    while (osal_queue_receive(s_mw.msg_queue, &msg, timeout_ms) == OSAL_OK) {
        mw_topic_t* topic = (mw_topic_t*)msg.topic;
        if (topic && topic->active) {
            osal_mutex_lock(s_mw.mutex, OSAL_WAIT_FOREVER);
            deliver_to_subscribers(topic, msg.data, msg.size);
            osal_mutex_unlock(s_mw.mutex);
        }
        processed++;
        timeout_ms = 0; /* Only first wait uses timeout */
    }
    
    return processed;
}

void mw_spin(void) {
    while (1) {
        mw_spin_once(OSAL_WAIT_FOREVER);
    }
}

bool mw_has_pending_messages(void) {
    return !osal_queue_is_empty(s_mw.msg_queue);
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

uint32_t mw_get_publish_count(void) {
    return s_mw.total_publish;
}

uint32_t mw_get_deliver_count(void) {
    return s_mw.total_deliver;
}

uint32_t mw_get_drop_count(void) {
    return s_mw.total_drop;
}

void mw_reset_stats(void) {
    s_mw.total_publish = 0;
    s_mw.total_deliver = 0;
    s_mw.total_drop = 0;
    
    for (int i = 0; i < s_mw.topic_count; i++) {
        s_mw.topics[i].publish_count = 0;
        s_mw.topics[i].deliver_count = 0;
    }
}