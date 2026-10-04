/**
 * @file osal_mock.c
 * @brief OSAL Mock Implementation for Unit Testing
 */

#include "osal/osal.h"
#include <string.h>
#include <stdlib.h>

/* Mock task storage */
typedef struct {
    osal_task_config_t config;
    bool created;
    bool running;
} mock_task_t;

static mock_task_t mock_tasks[16];
static int mock_task_count = 0;

/* Mock mutex storage */
typedef struct {
    bool created;
    bool locked;
} mock_mutex_t;

static mock_mutex_t mock_mutexes[16];
static int mock_mutex_count = 0;

void osal_mock_reset(void) {
    memset(mock_tasks, 0, sizeof(mock_tasks));
    memset(mock_mutexes, 0, sizeof(mock_mutexes));
    mock_task_count = 0;
    mock_mutex_count = 0;
}

osal_err_t osal_init(void) {
    osal_mock_reset();
    return OSAL_OK;
}

osal_err_t osal_deinit(void) {
    osal_mock_reset();
    return OSAL_OK;
}

/* Task functions */
osal_err_t osal_task_create(const osal_task_config_t* config, osal_task_handle_t* handle) {
    if (!config || !handle || mock_task_count >= 16) {
        return OSAL_ERR_INVALID_ARG;
    }
    
    mock_tasks[mock_task_count].config = *config;
    mock_tasks[mock_task_count].created = true;
    mock_tasks[mock_task_count].running = false;
    
    *handle = (osal_task_handle_t)(uintptr_t)(mock_task_count + 1);
    mock_task_count++;
    
    return OSAL_OK;
}

osal_err_t osal_task_delete(osal_task_handle_t handle) {
    int idx = (int)(uintptr_t)handle - 1;
    if (idx < 0 || idx >= mock_task_count) {
        return OSAL_ERR_INVALID_ARG;
    }
    mock_tasks[idx].created = false;
    mock_tasks[idx].running = false;
    return OSAL_OK;
}

void osal_task_delay_ms(osal_ms_t ms) {
    (void)ms;
}

/* Mutex functions */
osal_err_t osal_mutex_create(osal_mutex_handle_t* handle) {
    if (!handle || mock_mutex_count >= 16) {
        return OSAL_ERR_INVALID_ARG;
    }
    
    mock_mutexes[mock_mutex_count].created = true;
    mock_mutexes[mock_mutex_count].locked = false;
    
    *handle = (osal_mutex_handle_t)(uintptr_t)(mock_mutex_count + 1);
    mock_mutex_count++;
    
    return OSAL_OK;
}

osal_err_t osal_mutex_lock(osal_mutex_handle_t handle, osal_ms_t timeout) {
    (void)timeout;
    int idx = (int)(uintptr_t)handle - 1;
    if (idx < 0 || idx >= mock_mutex_count) {
        return OSAL_ERR_INVALID_ARG;
    }
    if (mock_mutexes[idx].locked) {
        return OSAL_ERR_BUSY;
    }
    mock_mutexes[idx].locked = true;
    return OSAL_OK;
}

osal_err_t osal_mutex_unlock(osal_mutex_handle_t handle) {
    int idx = (int)(uintptr_t)handle - 1;
    if (idx < 0 || idx >= mock_mutex_count) {
        return OSAL_ERR_INVALID_ARG;
    }
    mock_mutexes[idx].locked = false;
    return OSAL_OK;
}

osal_err_t osal_mutex_delete(osal_mutex_handle_t handle) {
    int idx = (int)(uintptr_t)handle - 1;
    if (idx < 0 || idx >= mock_mutex_count) {
        return OSAL_ERR_INVALID_ARG;
    }
    mock_mutexes[idx].created = false;
    mock_mutexes[idx].locked = false;
    return OSAL_OK;
}

/* Semaphore functions */
osal_err_t osal_sem_create(osal_sem_handle_t* handle, uint32_t initial, uint32_t max) {
    (void)initial;
    (void)max;
    if (handle) *handle = (osal_sem_handle_t)1;
    return OSAL_OK;
}

osal_err_t osal_sem_take(osal_sem_handle_t handle, osal_ms_t timeout) {
    (void)handle;
    (void)timeout;
    return OSAL_OK;
}

osal_err_t osal_sem_give(osal_sem_handle_t handle) {
    (void)handle;
    return OSAL_OK;
}

osal_err_t osal_sem_delete(osal_sem_handle_t handle) {
    (void)handle;
    return OSAL_OK;
}

/* Time functions */
osal_tick_t osal_get_tick_count(void) {
    return 0;
}

osal_ms_t osal_tick_to_ms(osal_tick_t ticks) {
    return ticks;
}

osal_tick_t osal_ms_to_tick(osal_ms_t ms) {
    return ms;
}

/* Logging mock */
void osal_log_write(osal_log_level_t level, const char *tag, const char *fmt, ...) {
    (void)level;
    (void)tag;
    (void)fmt;
}

void osal_log_set_level(const char *tag, osal_log_level_t level) {
    (void)tag;
    (void)level;
}
