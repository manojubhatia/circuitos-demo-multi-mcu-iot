/**
 * @file osal_sync.h
 * @brief OS Abstraction Layer - Synchronization Primitives

 */

#ifndef OSAL_SYNC_H
#define OSAL_SYNC_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Mutex API
 * ============================================================================ */

/**
 * @brief Create a mutex
 * @param handle Output handle
 * @return OSAL_OK on success
 */
osal_err_t osal_mutex_create(osal_mutex_handle_t* handle);

/**
 * @brief Create a recursive mutex
 * @param handle Output handle
 * @return OSAL_OK on success
 */
osal_err_t osal_mutex_create_recursive(osal_mutex_handle_t* handle);

/**
 * @brief Delete a mutex
 * @param handle Mutex handle
 * @return OSAL_OK on success
 */
osal_err_t osal_mutex_delete(osal_mutex_handle_t handle);

/**
 * @brief Lock a mutex
 * @param handle Mutex handle
 * @param timeout_ms Timeout in milliseconds (OSAL_WAIT_FOREVER for infinite)
 * @return OSAL_OK on success, OSAL_ERR_TIMEOUT on timeout
 */
osal_err_t osal_mutex_lock(osal_mutex_handle_t handle, osal_ms_t timeout_ms);

/**
 * @brief Unlock a mutex
 * @param handle Mutex handle
 * @return OSAL_OK on success
 */
osal_err_t osal_mutex_unlock(osal_mutex_handle_t handle);

/* ============================================================================
 * Semaphore API
 * ============================================================================ */

/**
 * @brief Create a binary semaphore
 * @param handle Output handle
 * @return OSAL_OK on success
 */
osal_err_t osal_sem_create_binary(osal_sem_handle_t* handle);

/**
 * @brief Create a counting semaphore
 * @param handle Output handle
 * @param max_count Maximum count
 * @param initial_count Initial count
 * @return OSAL_OK on success
 */
osal_err_t osal_sem_create_counting(osal_sem_handle_t* handle, uint32_t max_count, uint32_t initial_count);

/**
 * @brief Delete a semaphore
 * @param handle Semaphore handle
 * @return OSAL_OK on success
 */
osal_err_t osal_sem_delete(osal_sem_handle_t handle);

/**
 * @brief Take (wait for) a semaphore
 * @param handle Semaphore handle
 * @param timeout_ms Timeout in milliseconds
 * @return OSAL_OK on success, OSAL_ERR_TIMEOUT on timeout
 */
osal_err_t osal_sem_take(osal_sem_handle_t handle, osal_ms_t timeout_ms);

/**
 * @brief Give (signal) a semaphore
 * @param handle Semaphore handle
 * @return OSAL_OK on success
 */
osal_err_t osal_sem_give(osal_sem_handle_t handle);

/**
 * @brief Give a semaphore from ISR
 * @param handle Semaphore handle
 * @return OSAL_OK on success
 */
osal_err_t osal_sem_give_from_isr(osal_sem_handle_t handle);

/* ============================================================================
 * Critical Section API
 * ============================================================================ */

/**
 * @brief Enter critical section (disable interrupts)
 * @return State to pass to exit function
 */
uint32_t osal_critical_enter(void);

/**
 * @brief Exit critical section (restore interrupts)
 * @param state State from enter function
 */
void osal_critical_exit(uint32_t state);

/* ============================================================================
 * Helper Macros
 * ============================================================================ */

#define OSAL_CRITICAL_SECTION_BEGIN() \
    do { uint32_t _osal_cs_state = osal_critical_enter()

#define OSAL_CRITICAL_SECTION_END() \
    osal_critical_exit(_osal_cs_state); } while(0)

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SYNC_H */