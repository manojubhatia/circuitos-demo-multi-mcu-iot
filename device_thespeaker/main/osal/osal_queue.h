/**
 * @file osal_queue.h
 * @brief OS Abstraction Layer - Message Queue

 */

#ifndef OSAL_QUEUE_H
#define OSAL_QUEUE_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a message queue
 * @param handle Output handle
 * @param item_size Size of each item in bytes
 * @param queue_length Maximum number of items
 * @return OSAL_OK on success
 */
osal_err_t osal_queue_create(osal_queue_handle_t* handle, size_t item_size, size_t queue_length);

/**
 * @brief Delete a queue
 * @param handle Queue handle
 * @return OSAL_OK on success
 */
osal_err_t osal_queue_delete(osal_queue_handle_t handle);

/**
 * @brief Send item to queue (back)
 * @param handle Queue handle
 * @param item Pointer to item data
 * @param timeout_ms Timeout in milliseconds
 * @return OSAL_OK on success, OSAL_ERR_TIMEOUT on timeout
 */
osal_err_t osal_queue_send(osal_queue_handle_t handle, const void* item, osal_ms_t timeout_ms);

/**
 * @brief Send item to front of queue
 * @param handle Queue handle
 * @param item Pointer to item data
 * @param timeout_ms Timeout in milliseconds
 * @return OSAL_OK on success
 */
osal_err_t osal_queue_send_front(osal_queue_handle_t handle, const void* item, osal_ms_t timeout_ms);

/**
 * @brief Send item to queue from ISR
 * @param handle Queue handle
 * @param item Pointer to item data
 * @return OSAL_OK on success
 */
osal_err_t osal_queue_send_from_isr(osal_queue_handle_t handle, const void* item);

/**
 * @brief Receive item from queue
 * @param handle Queue handle
 * @param item Buffer to store received item
 * @param timeout_ms Timeout in milliseconds
 * @return OSAL_OK on success, OSAL_ERR_TIMEOUT on timeout
 */
osal_err_t osal_queue_receive(osal_queue_handle_t handle, void* item, osal_ms_t timeout_ms);

/**
 * @brief Peek at front item without removing
 * @param handle Queue handle
 * @param item Buffer to store item
 * @param timeout_ms Timeout in milliseconds
 * @return OSAL_OK on success
 */
osal_err_t osal_queue_peek(osal_queue_handle_t handle, void* item, osal_ms_t timeout_ms);

/**
 * @brief Get number of items in queue
 * @param handle Queue handle
 * @return Number of items
 */
size_t osal_queue_get_count(osal_queue_handle_t handle);

/**
 * @brief Get available space in queue
 * @param handle Queue handle
 * @return Number of free slots
 */
size_t osal_queue_get_free(osal_queue_handle_t handle);

/**
 * @brief Check if queue is empty
 * @param handle Queue handle
 * @return true if empty
 */
bool osal_queue_is_empty(osal_queue_handle_t handle);

/**
 * @brief Check if queue is full
 * @param handle Queue handle
 * @return true if full
 */
bool osal_queue_is_full(osal_queue_handle_t handle);

/**
 * @brief Reset/flush queue
 * @param handle Queue handle
 * @return OSAL_OK on success
 */
osal_err_t osal_queue_reset(osal_queue_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_QUEUE_H */