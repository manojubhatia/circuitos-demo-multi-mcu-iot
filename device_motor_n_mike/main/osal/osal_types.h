/**
 * @file osal_types.h
 * @brief OS Abstraction Layer Common Types

 */

#ifndef OSAL_TYPES_H
#define OSAL_TYPES_H

#include "osal_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(ESP_PLATFORM) || defined(IDF_VER)
#include "esp_err.h"
#else
typedef int esp_err_t;
#define ESP_OK    0
#define ESP_FAIL  (-1)
#define ESP_ERR_INVALID_ARG  0x0102
#define ESP_ERR_NO_MEM       0x0101
#define ESP_ERR_TIMEOUT      0x0107
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
typedef enum {
    OSAL_OK = 0,
    OSAL_ERR_INVALID_ARG,
    OSAL_ERR_NO_MEMORY,
    OSAL_ERR_TIMEOUT,
    OSAL_ERR_BUSY,
    OSAL_ERR_NOT_FOUND,
    OSAL_ERR_NOT_SUPPORTED,
    OSAL_ERR_FAILED
} osal_err_t;

/* Time types */
typedef uint32_t osal_tick_t;
typedef uint32_t osal_ms_t;
typedef uint64_t osal_us_t;

/* Special timeout values */
#define OSAL_WAIT_FOREVER   ((osal_ms_t)0xFFFFFFFF)
#define OSAL_NO_WAIT        ((osal_ms_t)0)

/* Handle types - opaque pointers for OS objects */
typedef void* osal_task_handle_t;
typedef void* osal_mutex_handle_t;
typedef void* osal_sem_handle_t;
typedef void* osal_queue_handle_t;
typedef void* osal_timer_handle_t;

/* Task function prototype */
typedef void (*osal_task_func_t)(void* arg);

/* Timer callback prototype */
typedef void (*osal_timer_callback_t)(void* arg);

/* Task priority levels (portable) */
typedef enum {
    OSAL_PRIORITY_IDLE = 0,
    OSAL_PRIORITY_LOW = 1,
    OSAL_PRIORITY_BELOW_NORMAL = 2,
    OSAL_PRIORITY_NORMAL = 3,
    OSAL_PRIORITY_ABOVE_NORMAL = 4,
    OSAL_PRIORITY_HIGH = 5,
    OSAL_PRIORITY_REALTIME = 6
} osal_priority_t;

/* Task state */
typedef enum {
    OSAL_TASK_RUNNING,
    OSAL_TASK_READY,
    OSAL_TASK_BLOCKED,
    OSAL_TASK_SUSPENDED,
    OSAL_TASK_DELETED
} osal_task_state_t;

/* Task configuration */
typedef struct {
    const char* name;
    osal_task_func_t func;
    void* arg;
    uint32_t stack_size;
    osal_priority_t priority;
    bool pinned_to_core;
    int core_id;
} osal_task_config_t;

/* Timer configuration */
typedef struct {
    const char* name;
    osal_ms_t period_ms;
    bool auto_reload;
    osal_timer_callback_t callback;
    void* arg;
} osal_timer_config_t;

#ifdef __cplusplus
}
#endif

#endif /* OSAL_TYPES_H */