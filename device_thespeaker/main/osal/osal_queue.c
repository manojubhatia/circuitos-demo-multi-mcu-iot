/**
 * @file osal_queue.c
 * @brief OS Abstraction Layer - Message Queue Implementation
 */

#include "osal_queue.h"
#include "osal_config.h"
#include <string.h>

#include "osal/osal.h"
#include "osal/osal_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

osal_err_t osal_queue_create(osal_queue_handle_t* handle, size_t item_size, size_t queue_length) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    *handle = (osal_queue_handle_t)xQueueCreate(queue_length, item_size);
    return *handle ? OSAL_OK : OSAL_ERR_NO_MEMORY;
}

osal_err_t osal_queue_delete(osal_queue_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    vQueueDelete((QueueHandle_t)handle);
    return OSAL_OK;
}

osal_err_t osal_queue_send(osal_queue_handle_t handle, const void* item, osal_ms_t timeout_ms) {
    if (!handle || !item) return OSAL_ERR_INVALID_ARG;
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueSendToBack((QueueHandle_t)handle, item, ticks) == pdTRUE ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

osal_err_t osal_queue_send_front(osal_queue_handle_t handle, const void* item, osal_ms_t timeout_ms) {
    if (!handle || !item) return OSAL_ERR_INVALID_ARG;
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueSendToFront((QueueHandle_t)handle, item, ticks) == pdTRUE ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

osal_err_t osal_queue_send_from_isr(osal_queue_handle_t handle, const void* item) {
    if (!handle || !item) return OSAL_ERR_INVALID_ARG;
    BaseType_t higher_prio_woken = pdFALSE;
    BaseType_t ret = xQueueSendToBackFromISR((QueueHandle_t)handle, item, &higher_prio_woken);
    portYIELD_FROM_ISR(higher_prio_woken);
    return ret == pdTRUE ? OSAL_OK : OSAL_ERR_FAILED;
}

osal_err_t osal_queue_receive(osal_queue_handle_t handle, void* item, osal_ms_t timeout_ms) {
    if (!handle || !item) return OSAL_ERR_INVALID_ARG;
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueueReceive((QueueHandle_t)handle, item, ticks) == pdTRUE ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

osal_err_t osal_queue_peek(osal_queue_handle_t handle, void* item, osal_ms_t timeout_ms) {
    if (!handle || !item) return OSAL_ERR_INVALID_ARG;
    TickType_t ticks = (timeout_ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xQueuePeek((QueueHandle_t)handle, item, ticks) == pdTRUE ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

size_t osal_queue_get_count(osal_queue_handle_t handle) {
    if (!handle) return 0;
    return uxQueueMessagesWaiting((QueueHandle_t)handle);
}

size_t osal_queue_get_free(osal_queue_handle_t handle) {
    if (!handle) return 0;
    return uxQueueSpacesAvailable((QueueHandle_t)handle);
}

bool osal_queue_is_empty(osal_queue_handle_t handle) {
    return osal_queue_get_count(handle) == 0;
}

bool osal_queue_is_full(osal_queue_handle_t handle) {
    return osal_queue_get_free(handle) == 0;
}

osal_err_t osal_queue_reset(osal_queue_handle_t handle) {
    if (!handle) return OSAL_ERR_INVALID_ARG;
    xQueueReset((QueueHandle_t)handle);
    return OSAL_OK;
}

