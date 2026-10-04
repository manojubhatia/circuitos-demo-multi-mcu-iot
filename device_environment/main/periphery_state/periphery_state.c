/**
 * @file periphery_state.c
 * @brief Enhanced IO State Manager - Implementation

 */

#include "periphery_state.h"
#include "../osal/osal.h"
#include "../middleware/middleware.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

typedef struct {
    periphery_change_callback_t callback;
    void* user_data;
    bool active;
} io_callback_entry_t;

typedef struct {
    periphery_value_t threshold;
    periphery_threshold_callback_t callback;
    void* user_data;
    bool above_threshold;
    bool active;
} periphery_threshold_entry_t;

struct periphery_signal_s {
    char name[PERIPHERY_STATE_MAX_NAME_LEN];
    char component[PERIPHERY_STATE_MAX_NAME_LEN];
    periphery_type_t type;
    periphery_direction_t direction;
    periphery_state_t state;
    bool active;
    
    /* Callbacks */
    io_callback_entry_t callbacks[PERIPHERY_STATE_MAX_CALLBACKS];
    periphery_threshold_entry_t thresholds[PERIPHERY_STATE_MAX_CALLBACKS];
    
    /* Middleware integration */
    mw_publisher_handle_t publisher;
    mw_subscriber_handle_t subscriber;
    char publish_topic[PERIPHERY_STATE_MAX_NAME_LEN];
    char subscribe_topic[PERIPHERY_STATE_MAX_NAME_LEN];
};

/* ============================================================================
 * Static Data
 * ============================================================================ */

static struct {
    bool initialized;
    struct periphery_signal_s signals[PERIPHERY_STATE_MAX_SIGNALS];
    int signal_count;
    osal_mutex_handle_t mutex;
    
    /* Global change callbacks (for "*" pattern) */
    io_callback_entry_t global_callbacks[PERIPHERY_STATE_MAX_CALLBACKS];
} s_io;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static struct periphery_signal_s* find_signal(const char* name) {
    for (int i = 0; i < s_io.signal_count; i++) {
        if (s_io.signals[i].active && strcmp(s_io.signals[i].name, name) == 0) {
            return &s_io.signals[i];
        }
    }
    return NULL;
}

static size_t type_size(periphery_type_t type) {
    switch (type) {
        case PERIPHERY_TYPE_BOOL:   return sizeof(bool);
        case PERIPHERY_TYPE_INT8:   return sizeof(int8_t);
        case PERIPHERY_TYPE_UINT8:  return sizeof(uint8_t);
        case PERIPHERY_TYPE_INT16:  return sizeof(int16_t);
        case PERIPHERY_TYPE_UINT16: return sizeof(uint16_t);
        case PERIPHERY_TYPE_INT32:  return sizeof(int32_t);
        case PERIPHERY_TYPE_UINT32: return sizeof(uint32_t);
        case PERIPHERY_TYPE_INT64:  return sizeof(int64_t);
        case PERIPHERY_TYPE_UINT64: return sizeof(uint64_t);
        case PERIPHERY_TYPE_FLOAT:  return sizeof(float);
        case PERIPHERY_TYPE_DOUBLE: return sizeof(double);
        default:             return PERIPHERY_STATE_MAX_DATA_SIZE;
    }
}

static bool values_equal(periphery_type_t type, const periphery_value_t* a, const periphery_value_t* b) {
    switch (type) {
        case PERIPHERY_TYPE_BOOL:   return a->b == b->b;
        case PERIPHERY_TYPE_INT8:   return a->i8 == b->i8;
        case PERIPHERY_TYPE_UINT8:  return a->u8 == b->u8;
        case PERIPHERY_TYPE_INT16:  return a->i16 == b->i16;
        case PERIPHERY_TYPE_UINT16: return a->u16 == b->u16;
        case PERIPHERY_TYPE_INT32:  return a->i32 == b->i32;
        case PERIPHERY_TYPE_UINT32: return a->u32 == b->u32;
        case PERIPHERY_TYPE_INT64:  return a->i64 == b->i64;
        case PERIPHERY_TYPE_UINT64: return a->u64 == b->u64;
        case PERIPHERY_TYPE_FLOAT:  return a->f == b->f;
        case PERIPHERY_TYPE_DOUBLE: return a->d == b->d;
        case PERIPHERY_TYPE_STRING: return strcmp(a->str, b->str) == 0;
        default:             return memcmp(a, b, sizeof(periphery_value_t)) == 0;
    }
}

static double value_to_double(periphery_type_t type, const periphery_value_t* v) {
    switch (type) {
        case PERIPHERY_TYPE_BOOL:   return v->b ? 1.0 : 0.0;
        case PERIPHERY_TYPE_INT8:   return (double)v->i8;
        case PERIPHERY_TYPE_UINT8:  return (double)v->u8;
        case PERIPHERY_TYPE_INT16:  return (double)v->i16;
        case PERIPHERY_TYPE_UINT16: return (double)v->u16;
        case PERIPHERY_TYPE_INT32:  return (double)v->i32;
        case PERIPHERY_TYPE_UINT32: return (double)v->u32;
        case PERIPHERY_TYPE_INT64:  return (double)v->i64;
        case PERIPHERY_TYPE_UINT64: return (double)v->u64;
        case PERIPHERY_TYPE_FLOAT:  return (double)v->f;
        case PERIPHERY_TYPE_DOUBLE: return v->d;
        default:             return 0.0;
    }
}

static void invoke_callbacks(struct periphery_signal_s* sig) {
    /* Signal-specific callbacks */
    for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
        if (sig->callbacks[i].active && sig->callbacks[i].callback) {
            sig->callbacks[i].callback(
                sig->name,
                &sig->state.value,
                &sig->state.prev_value,
                sig->callbacks[i].user_data
            );
        }
    }
    
    /* Global callbacks */
    for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
        if (s_io.global_callbacks[i].active && s_io.global_callbacks[i].callback) {
            s_io.global_callbacks[i].callback(
                sig->name,
                &sig->state.value,
                &sig->state.prev_value,
                s_io.global_callbacks[i].user_data
            );
        }
    }
}

static void check_thresholds(struct periphery_signal_s* sig) {
    double current = value_to_double(sig->type, &sig->state.value);
    
    for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
        periphery_threshold_entry_t* te = &sig->thresholds[i];
        if (!te->active || !te->callback) continue;
        
        double threshold = value_to_double(sig->type, &te->threshold);
        bool now_above = current >= threshold;
        
        if (now_above != te->above_threshold) {
            te->callback(sig->name, &sig->state.value, &te->threshold, now_above, te->user_data);
            te->above_threshold = now_above;
        }
    }
}

static void auto_publish(struct periphery_signal_s* sig) {
    if (!sig->publisher) return;
    
    /* Publish the value with appropriate message type */
    switch (sig->type) {
        case PERIPHERY_TYPE_FLOAT: {
            mw_msg_float_t msg = { .data = sig->state.value.f };
            mw_publish(sig->publisher, &msg);
            break;
        }
        case PERIPHERY_TYPE_INT32: {
            mw_msg_int32_t msg = { .data = sig->state.value.i32 };
            mw_publish(sig->publisher, &msg);
            break;
        }
        case PERIPHERY_TYPE_BOOL: {
            mw_msg_bool_t msg = { .data = sig->state.value.b };
            mw_publish(sig->publisher, &msg);
            break;
        }
        default:
            /* Generic bytes publish */
            mw_publish(sig->publisher, &sig->state.value);
            break;
    }
}

/* ============================================================================
 * Initialization
 * ============================================================================ */

int periphery_state_init(void) {
    if (s_io.initialized) return 0;
    
    memset(&s_io, 0, sizeof(s_io));
    
    if (osal_mutex_create(&s_io.mutex) != OSAL_OK) {
        return -1;
    }
    
    s_io.initialized = true;
    return 0;
}

void periphery_state_shutdown(void) {
    if (!s_io.initialized) return;
    
    /* Cleanup publishers and subscribers */
    for (int i = 0; i < s_io.signal_count; i++) {
        if (s_io.signals[i].publisher) {
            mw_destroy_publisher(s_io.signals[i].publisher);
        }
        if (s_io.signals[i].subscriber) {
            mw_destroy_subscriber(s_io.signals[i].subscriber);
        }
    }
    
    osal_mutex_delete(s_io.mutex);
    memset(&s_io, 0, sizeof(s_io));
}

bool periphery_state_is_initialized(void) {
    return s_io.initialized;
}

/* ============================================================================
 * Signal Registration
 * ============================================================================ */

periphery_signal_handle_t periphery_state_register(
    const char* name,
    periphery_type_t type,
    periphery_direction_t direction,
    const char* component
) {
    if (!s_io.initialized || !name) return NULL;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    /* Check if already exists */
    struct periphery_signal_s* existing = find_signal(name);
    if (existing) {
        osal_mutex_unlock(s_io.mutex);
        return existing;
    }
    
    /* Allocate new signal */
    if (s_io.signal_count >= PERIPHERY_STATE_MAX_SIGNALS) {
        osal_mutex_unlock(s_io.mutex);
        return NULL;
    }
    
    struct periphery_signal_s* sig = &s_io.signals[s_io.signal_count++];
    memset(sig, 0, sizeof(struct periphery_signal_s));
    
    strncpy(sig->name, name, PERIPHERY_STATE_MAX_NAME_LEN - 1);
    if (component) {
        strncpy(sig->component, component, PERIPHERY_STATE_MAX_NAME_LEN - 1);
    }
    sig->type = type;
    sig->direction = direction;
    sig->active = true;
    
    osal_mutex_unlock(s_io.mutex);
    return sig;
}

void periphery_state_unregister(const char* name) {
    if (!s_io.initialized || !name) return;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(name);
    if (sig) {
        if (sig->publisher) mw_destroy_publisher(sig->publisher);
        if (sig->subscriber) mw_destroy_subscriber(sig->subscriber);
        sig->active = false;
    }
    
    osal_mutex_unlock(s_io.mutex);
}

periphery_signal_handle_t periphery_state_find(const char* name) {
    if (!s_io.initialized || !name) return NULL;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    struct periphery_signal_s* sig = find_signal(name);
    osal_mutex_unlock(s_io.mutex);
    
    return sig;
}

int periphery_state_get_count(void) {
    return s_io.signal_count;
}

periphery_signal_handle_t periphery_state_get_by_index(int index) {
    if (index < 0 || index >= s_io.signal_count) return NULL;
    return &s_io.signals[index];
}

const char* periphery_state_get_name(periphery_signal_handle_t handle) {
    struct periphery_signal_s* sig = (struct periphery_signal_s*)handle;
    return sig ? sig->name : NULL;
}

periphery_type_t periphery_state_get_type(periphery_signal_handle_t handle) {
    struct periphery_signal_s* sig = (struct periphery_signal_s*)handle;
    return sig ? sig->type : PERIPHERY_TYPE_INT32;
}

periphery_direction_t periphery_state_get_direction(periphery_signal_handle_t handle) {
    struct periphery_signal_s* sig = (struct periphery_signal_s*)handle;
    return sig ? sig->direction : PERIPHERY_DIR_OUTPUT;
}

/* ============================================================================
 * Value Access - Generic
 * ============================================================================ */

int periphery_state_set(const char* name, const void* value, size_t size) {
    if (!s_io.initialized || !name || !value) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(name);
    if (!sig) {
        osal_mutex_unlock(s_io.mutex);
        return -1;
    }
    
    /* Store previous value */
    sig->state.prev_value = sig->state.value;
    sig->state.prev_timestamp_us = sig->state.timestamp_us;
    
    /* Update value */
    size_t copy_size = size < sizeof(periphery_value_t) ? size : sizeof(periphery_value_t);
    memcpy(&sig->state.value, value, copy_size);
    sig->state.timestamp_us = osal_get_time_us();
    sig->state.update_count++;
    sig->state.valid = true;
    
    /* Check for change */
    sig->state.changed = !values_equal(sig->type, &sig->state.value, &sig->state.prev_value);
    
    if (sig->state.changed) {
        invoke_callbacks(sig);
        check_thresholds(sig);
        auto_publish(sig);
    }
    
    osal_mutex_unlock(s_io.mutex);
    return 0;
}

int periphery_state_get(const char* name, void* value, size_t size) {
    if (!s_io.initialized || !name || !value) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(name);
    if (!sig) {
        osal_mutex_unlock(s_io.mutex);
        return -1;
    }
    
    size_t copy_size = size < sizeof(periphery_value_t) ? size : sizeof(periphery_value_t);
    memcpy(value, &sig->state.value, copy_size);
    
    osal_mutex_unlock(s_io.mutex);
    return 0;
}

int periphery_state_get_state(const char* name, periphery_state_t* state) {
    if (!s_io.initialized || !name || !state) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(name);
    if (!sig) {
        osal_mutex_unlock(s_io.mutex);
        return -1;
    }
    
    *state = sig->state;
    
    osal_mutex_unlock(s_io.mutex);
    return 0;
}

/* ============================================================================
 * Typed Convenience Functions
 * ============================================================================ */

int periphery_state_set_bool(const char* name, bool value) {
    periphery_value_t v = { .b = value };
    return periphery_state_set(name, &v, sizeof(bool));
}

bool periphery_state_get_bool(const char* name) {
    periphery_value_t v = {0};
    periphery_state_get(name, &v, sizeof(periphery_value_t));
    return v.b;
}

int periphery_state_set_int(const char* name, int32_t value) {
    periphery_value_t v = { .i32 = value };
    return periphery_state_set(name, &v, sizeof(int32_t));
}

int32_t periphery_state_get_int(const char* name) {
    periphery_value_t v = {0};
    periphery_state_get(name, &v, sizeof(periphery_value_t));
    return v.i32;
}

int periphery_state_set_uint(const char* name, uint32_t value) {
    periphery_value_t v = { .u32 = value };
    return periphery_state_set(name, &v, sizeof(uint32_t));
}

uint32_t periphery_state_get_uint(const char* name) {
    periphery_value_t v = {0};
    periphery_state_get(name, &v, sizeof(periphery_value_t));
    return v.u32;
}

int periphery_state_set_float(const char* name, float value) {
    periphery_value_t v = { .f = value };
    return periphery_state_set(name, &v, sizeof(float));
}

float periphery_state_get_float(const char* name) {
    periphery_value_t v = {0};
    periphery_state_get(name, &v, sizeof(periphery_value_t));
    return v.f;
}

int periphery_state_set_double(const char* name, double value) {
    periphery_value_t v = { .d = value };
    return periphery_state_set(name, &v, sizeof(double));
}

double periphery_state_get_double(const char* name) {
    periphery_value_t v = {0};
    periphery_state_get(name, &v, sizeof(periphery_value_t));
    return v.d;
}

int periphery_state_set_string(const char* name, const char* value) {
    periphery_value_t v = {0};
    if (value) {
        strncpy(v.str, value, PERIPHERY_STATE_MAX_DATA_SIZE - 1);
    }
    return periphery_state_set(name, &v, PERIPHERY_STATE_MAX_DATA_SIZE);
}

const char* periphery_state_get_string(const char* name) {
    static periphery_value_t v;
    periphery_state_get(name, &v, sizeof(periphery_value_t));
    return v.str;
}

/* ============================================================================
 * Change Detection & Callbacks
 * ============================================================================ */

int periphery_state_on_change(const char* name, periphery_change_callback_t callback, void* user_data) {
    if (!s_io.initialized || !callback) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    if (strcmp(name, "*") == 0) {
        /* Global callback */
        for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
            if (!s_io.global_callbacks[i].active) {
                s_io.global_callbacks[i].callback = callback;
                s_io.global_callbacks[i].user_data = user_data;
                s_io.global_callbacks[i].active = true;
                osal_mutex_unlock(s_io.mutex);
                return 0;
            }
        }
    } else {
        struct periphery_signal_s* sig = find_signal(name);
        if (!sig) {
            osal_mutex_unlock(s_io.mutex);
            return -1;
        }
        
        for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
            if (!sig->callbacks[i].active) {
                sig->callbacks[i].callback = callback;
                sig->callbacks[i].user_data = user_data;
                sig->callbacks[i].active = true;
                osal_mutex_unlock(s_io.mutex);
                return 0;
            }
        }
    }
    
    osal_mutex_unlock(s_io.mutex);
    return -1;
}

void periphery_state_remove_callback(const char* name, periphery_change_callback_t callback) {
    if (!s_io.initialized) return;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    if (strcmp(name, "*") == 0) {
        for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
            if (s_io.global_callbacks[i].callback == callback) {
                s_io.global_callbacks[i].active = false;
            }
        }
    } else {
        struct periphery_signal_s* sig = find_signal(name);
        if (sig) {
            for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
                if (sig->callbacks[i].callback == callback) {
                    sig->callbacks[i].active = false;
                }
            }
        }
    }
    
    osal_mutex_unlock(s_io.mutex);
}

int periphery_state_on_threshold(
    const char* name,
    periphery_value_t threshold,
    periphery_threshold_callback_t callback,
    void* user_data
) {
    if (!s_io.initialized || !name || !callback) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(name);
    if (!sig) {
        osal_mutex_unlock(s_io.mutex);
        return -1;
    }
    
    for (int i = 0; i < PERIPHERY_STATE_MAX_CALLBACKS; i++) {
        if (!sig->thresholds[i].active) {
            sig->thresholds[i].threshold = threshold;
            sig->thresholds[i].callback = callback;
            sig->thresholds[i].user_data = user_data;
            sig->thresholds[i].above_threshold = value_to_double(sig->type, &sig->state.value) >= 
                                                  value_to_double(sig->type, &threshold);
            sig->thresholds[i].active = true;
            osal_mutex_unlock(s_io.mutex);
            return 0;
        }
    }
    
    osal_mutex_unlock(s_io.mutex);
    return -1;
}

bool periphery_state_has_changed(const char* name) {
    if (!s_io.initialized || !name) return false;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    struct periphery_signal_s* sig = find_signal(name);
    bool changed = sig ? sig->state.changed : false;
    osal_mutex_unlock(s_io.mutex);
    
    return changed;
}

void periphery_state_clear_changed(const char* name) {
    if (!s_io.initialized || !name) return;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    struct periphery_signal_s* sig = find_signal(name);
    if (sig) sig->state.changed = false;
    osal_mutex_unlock(s_io.mutex);
}

uint64_t periphery_state_get_age_us(const char* name) {
    if (!s_io.initialized || !name) return UINT64_MAX;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    struct periphery_signal_s* sig = find_signal(name);
    uint64_t age = UINT64_MAX;
    if (sig && sig->state.valid) {
        age = osal_get_time_us() - sig->state.timestamp_us;
    }
    osal_mutex_unlock(s_io.mutex);
    
    return age;
}

/* ============================================================================
 * Middleware Integration
 * ============================================================================ */

int periphery_state_enable_autopublish(const char* signal_name, const char* topic_name) {
    if (!s_io.initialized || !signal_name || !topic_name) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(signal_name);
    if (!sig) {
        osal_mutex_unlock(s_io.mutex);
        return -1;
    }
    
    if (sig->publisher) {
        mw_destroy_publisher(sig->publisher);
    }
    
    strncpy(sig->publish_topic, topic_name, PERIPHERY_STATE_MAX_NAME_LEN - 1);
    int ret = mw_create_publisher(topic_name, type_size(sig->type), MW_QOS_BEST_EFFORT, &sig->publisher);
    
    osal_mutex_unlock(s_io.mutex);
    return ret;
}

void periphery_state_disable_autopublish(const char* signal_name) {
    if (!s_io.initialized || !signal_name) return;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(signal_name);
    if (sig && sig->publisher) {
        mw_destroy_publisher(sig->publisher);
        sig->publisher = NULL;
        sig->publish_topic[0] = '\0';
    }
    
    osal_mutex_unlock(s_io.mutex);
}

static void autosubscribe_callback(const char* topic, const void* msg, size_t size, void* user_data) {
    struct periphery_signal_s* sig = (struct periphery_signal_s*)user_data;
    if (sig && sig->active) {
        periphery_state_set(sig->name, msg, size);
    }
}

int periphery_state_enable_autosubscribe(const char* signal_name, const char* topic_name) {
    if (!s_io.initialized || !signal_name || !topic_name) return -1;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(signal_name);
    if (!sig) {
        osal_mutex_unlock(s_io.mutex);
        return -1;
    }
    
    if (sig->subscriber) {
        mw_destroy_subscriber(sig->subscriber);
    }
    
    strncpy(sig->subscribe_topic, topic_name, PERIPHERY_STATE_MAX_NAME_LEN - 1);
    int ret = mw_create_subscriber(topic_name, autosubscribe_callback, sig, MW_QOS_BEST_EFFORT, &sig->subscriber);
    
    osal_mutex_unlock(s_io.mutex);
    return ret;
}

void periphery_state_disable_autosubscribe(const char* signal_name) {
    if (!s_io.initialized || !signal_name) return;
    
    osal_mutex_lock(s_io.mutex, OSAL_WAIT_FOREVER);
    
    struct periphery_signal_s* sig = find_signal(signal_name);
    if (sig && sig->subscriber) {
        mw_destroy_subscriber(sig->subscriber);
        sig->subscriber = NULL;
        sig->subscribe_topic[0] = '\0';
    }
    
    osal_mutex_unlock(s_io.mutex);
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

void periphery_state_process(void) {
    /* Currently callbacks are invoked synchronously on set() */
    /* This function can be used for deferred callback processing if needed */
}

void periphery_state_dump(void) {
#if OSAL_USE_FREERTOS
    extern int printf(const char*, ...);
#endif
    
    printf("\n=== IO State Dump ===\n");
    printf("Signals: %d/%d\n\n", s_io.signal_count, PERIPHERY_STATE_MAX_SIGNALS);
    
    for (int i = 0; i < s_io.signal_count; i++) {
        struct periphery_signal_s* sig = &s_io.signals[i];
        if (!sig->active) continue;
        
        printf("[%s] (%s)\n", sig->name, sig->component);
        printf("  Type: %d, Direction: %d, Valid: %s\n",
               sig->type, sig->direction, sig->state.valid ? "yes" : "no");
        
        if (sig->state.valid) {
            switch (sig->type) {
                case PERIPHERY_TYPE_BOOL:   printf("  Value: %s\n", sig->state.value.b ? "true" : "false"); break;
                case PERIPHERY_TYPE_INT32:  printf("  Value: %ld\n", (long)sig->state.value.i32); break;
                case PERIPHERY_TYPE_UINT32: printf("  Value: %lu\n", (unsigned long)sig->state.value.u32); break;
                case PERIPHERY_TYPE_FLOAT:  printf("  Value: %.3f\n", sig->state.value.f); break;
                case PERIPHERY_TYPE_DOUBLE: printf("  Value: %.6f\n", sig->state.value.d); break;
                case PERIPHERY_TYPE_STRING: printf("  Value: \"%s\"\n", sig->state.value.str); break;
                default:             printf("  Value: [binary]\n"); break;
            }
        }
        
        printf("  Updates: %lu, Age: %llu us\n",
               (unsigned long)sig->state.update_count,
               sig->state.valid ? (osal_get_time_us() - sig->state.timestamp_us) : 0ULL);
        printf("\n");
    }
}

size_t periphery_state_get_memory_usage(void) {
    return sizeof(s_io);
}