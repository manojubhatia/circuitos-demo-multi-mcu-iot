/**
 * @file osal_task.h
 * @brief OS Abstraction Layer - Task/Thread Management

 */

#ifndef OSAL_TASK_H
#define OSAL_TASK_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new task
 * @param config Task configuration
 * @param handle Output handle for the created task
 * @return OSAL_OK on success
 */
osal_err_t osal_task_create(const osal_task_config_t* config, osal_task_handle_t* handle);

/**
 * @brief Delete a task
 * @param handle Task handle (NULL for current task)
 * @return OSAL_OK on success
 */
osal_err_t osal_task_delete(osal_task_handle_t handle);

/**
 * @brief Suspend a task
 * @param handle Task handle
 * @return OSAL_OK on success
 */
osal_err_t osal_task_suspend(osal_task_handle_t handle);

/**
 * @brief Resume a suspended task
 * @param handle Task handle
 * @return OSAL_OK on success
 */
osal_err_t osal_task_resume(osal_task_handle_t handle);

/**
 * @brief Get current task handle
 * @return Current task handle
 */
osal_task_handle_t osal_task_get_current(void);

/**
 * @brief Yield to other tasks
 */
void osal_task_yield(void);

/**
 * @brief Delay current task
 * @param ms Delay in milliseconds
 */
void osal_task_delay_ms(osal_ms_t ms);

/**
 * @brief Delay current task until absolute time
 * @param wake_tick Tick count to wake at
 */
void osal_task_delay_until(osal_tick_t* wake_tick, osal_ms_t period_ms);

/**
 * @brief Get task state
 * @param handle Task handle
 * @return Task state
 */
osal_task_state_t osal_task_get_state(osal_task_handle_t handle);

/**
 * @brief Set task priority
 * @param handle Task handle
 * @param priority New priority
 * @return OSAL_OK on success
 */
osal_err_t osal_task_set_priority(osal_task_handle_t handle, osal_priority_t priority);

/**
 * @brief Get task priority
 * @param handle Task handle
 * @return Task priority
 */
osal_priority_t osal_task_get_priority(osal_task_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_TASK_H */