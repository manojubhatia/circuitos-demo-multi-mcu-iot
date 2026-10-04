/**
 * @file osal_sync.c
 * @brief OS Abstraction Layer - Synchronization Implementation
 */

#include "osal_sync.h"
#include "osal_config.h"

#include "osal/osal.h"
#include "osal/osal_sync.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

/* Critical section spinlock - must be declared before use */
static portMUX_TYPE s_osal_mux = portMUX_INITIALIZER_UNLOCKED;

osal_err_t osal_mutex_create(osal_mutex_handle_t* handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    *handle = (osal_mutex_handle_t)xSemaphoreCreateMutex();
    return *handle ? OSAL_OK : OSAL_ERR_NO_MEMORY;
}

osal_err_t osal_mutex_create_recursive(osal_mutex_handle_t* handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    *handle = (osal_mutex_handle_t)xSemaphoreCreateRecursiveMutex();
    return *handle ? OSAL_OK : OSAL_ERR_NO_MEMORY;
}

osal_err_t osal_mutex_delete(osal_mutex_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    vSemaphoreDelete((SemaphoreHandle_t)handle);
    return OSAL_OK;
}

osal_err_t osal_mutex_lock(osal_mutex_handle_t handle, osal_ms_t timeout_ms) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)handle, ticks) == pdTRUE ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

osal_err_t osal_mutex_unlock(osal_mutex_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    return xSemaphoreGive((SemaphoreHandle_t)handle) == pdTRUE ? OSAL_OK : OSAL_ERR_FAILED;
}

osal_err_t osal_sem_create_binary(osal_sem_handle_t* handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    *handle = (osal_sem_handle_t)xSemaphoreCreateBinary();
    return *handle ? OSAL_OK : OSAL_ERR_NO_MEMORY;
}

osal_err_t osal_sem_create_counting(osal_sem_handle_t* handle, uint32_t max_count, uint32_t initial_count) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    *handle = (osal_sem_handle_t)xSemaphoreCreateCounting(max_count, initial_count);
    return *handle ? OSAL_OK : OSAL_ERR_NO_MEMORY;
}

osal_err_t osal_sem_delete(osal_sem_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    vSemaphoreDelete((SemaphoreHandle_t)handle);
    return OSAL_OK;
}

osal_err_t osal_sem_take(osal_sem_handle_t handle, osal_ms_t timeout_ms) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake((SemaphoreHandle_t)handle, ticks) == pdTRUE ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

osal_err_t osal_sem_give(osal_sem_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    return xSemaphoreGive((SemaphoreHandle_t)handle) == pdTRUE ? OSAL_OK : OSAL_ERR_FAILED;
}

osal_err_t osal_sem_give_from_isr(osal_sem_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    BaseType_t higher_prio_woken = pdFALSE;
    BaseType_t ret = xSemaphoreGiveFromISR((SemaphoreHandle_t)handle, &higher_prio_woken);
    portYIELD_FROM_ISR(higher_prio_woken);
    return ret == pdTRUE ? OSAL_OK : OSAL_ERR_FAILED;
}

uint32_t osal_critical_enter(void) {
    portENTER_CRITICAL(&s_osal_mux);
    return 0;
}

void osal_critical_exit(uint32_t state) {
    (void)state;
    portEXIT_CRITICAL(&s_osal_mux);
}

