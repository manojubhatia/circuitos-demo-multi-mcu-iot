/**
 * @file osal_task.c
 * @brief OS Abstraction Layer - Task Implementation
 */

#include "osal_task.h"
#include "osal_config.h"

#include "osal/osal.h"
#include "osal/osal_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static UBaseType_t map_priority(osal_priority_t prio) {
    switch (prio) {
        case OSAL_PRIORITY_IDLE:        return tskIDLE_PRIORITY;
        case OSAL_PRIORITY_LOW:         return tskIDLE_PRIORITY + 1;
        case OSAL_PRIORITY_BELOW_NORMAL: return tskIDLE_PRIORITY + 2;
        case OSAL_PRIORITY_NORMAL:      return tskIDLE_PRIORITY + 3;
        case OSAL_PRIORITY_ABOVE_NORMAL: return tskIDLE_PRIORITY + 4;
        case OSAL_PRIORITY_HIGH:        return configMAX_PRIORITIES - 2;
        case OSAL_PRIORITY_REALTIME:    return configMAX_PRIORITIES - 1;
        default:                        return tskIDLE_PRIORITY + 3;
    }
}

osal_err_t osal_task_create(const osal_task_config_t* config, osal_task_handle_t* handle) {
    if (!config || !config->func || !handle) {
        return OSAL_ERR_INVALID_ARG;
    }
    
    BaseType_t ret;
    (void)config->pinned_to_core;
    (void)config->core_id;
    ret = xTaskCreate(
        (TaskFunction_t)config->func,
        config->name ? config->name : "osal_task",
        config->stack_size,
        config->arg,
        map_priority(config->priority),
        (TaskHandle_t*)handle
    );
    
    return (ret == pdPASS) ? OSAL_OK : OSAL_ERR_NO_MEMORY;
}

osal_err_t osal_task_delete(osal_task_handle_t handle) {
    vTaskDelete((TaskHandle_t)handle);
    return OSAL_OK;
}

osal_err_t osal_task_suspend(osal_task_handle_t handle) {
    vTaskSuspend((TaskHandle_t)handle);
    return OSAL_OK;
}

osal_err_t osal_task_resume(osal_task_handle_t handle) {
    vTaskResume((TaskHandle_t)handle);
    return OSAL_OK;
}

osal_task_handle_t osal_task_get_current(void) {
    return (osal_task_handle_t)xTaskGetCurrentTaskHandle();
}

void osal_task_yield(void) {
    taskYIELD();
}

void osal_task_delay_ms(osal_ms_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void osal_task_delay_until(osal_tick_t* wake_tick, osal_ms_t period_ms) {
    TickType_t last = *wake_tick;
    vTaskDelayUntil(&last, pdMS_TO_TICKS(period_ms));
    *wake_tick = last;
}

osal_task_state_t osal_task_get_state(osal_task_handle_t handle) {
    eTaskState state = eTaskGetState((TaskHandle_t)handle);
    switch (state) {
        case eRunning:   return OSAL_TASK_RUNNING;
        case eReady:     return OSAL_TASK_READY;
        case eBlocked:   return OSAL_TASK_BLOCKED;
        case eSuspended: return OSAL_TASK_SUSPENDED;
        case eDeleted:   return OSAL_TASK_DELETED;
        default:         return OSAL_TASK_READY;
    }
}

osal_err_t osal_task_set_priority(osal_task_handle_t handle, osal_priority_t priority) {
    vTaskPrioritySet((TaskHandle_t)handle, map_priority(priority));
    return OSAL_OK;
}

osal_priority_t osal_task_get_priority(osal_task_handle_t handle) {
    UBaseType_t prio = uxTaskPriorityGet((TaskHandle_t)handle);
    if (prio <= tskIDLE_PRIORITY) return OSAL_PRIORITY_IDLE;
    if (prio >= configMAX_PRIORITIES - 1) return OSAL_PRIORITY_REALTIME;
    if (prio >= configMAX_PRIORITIES - 2) return OSAL_PRIORITY_HIGH;
    return OSAL_PRIORITY_NORMAL;
}
