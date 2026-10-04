/**
 * @file osal.h
 * @brief OS Abstraction Layer - Main Header

 * 
 * Provides a unified API for common OS primitives that works across:
 * - FreeRTOS (ESP-IDF, Arduino ESP32)
 * - Zephyr RTOS
 * - Linux/POSIX (Yocto, Raspberry Pi)
 * - Bare-metal (single-threaded)
 * 
 * Usage:
 *   #include "osal/osal.h"
 *   
 *   // Create a task
 *   osal_task_config_t cfg = {
 *       .name = "my_task",
 *       .func = my_task_func,
 *       .stack_size = 4096,
 *       .priority = OSAL_PRIORITY_NORMAL
 *   };
 *   osal_task_handle_t task;
 *   osal_task_create(&cfg, &task);
 */

#ifndef OSAL_H
#define OSAL_H

#include "osal_config.h"
#include "osal_types.h"
#include "osal_task.h"
#include "osal_sync.h"
#include "osal_queue.h"
#include "osal_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Time Functions
 * ============================================================================ */

/**
 * @brief Get current tick count
 * @return Current tick count
 */
osal_tick_t osal_get_tick_count(void);

/**
 * @brief Get time in milliseconds since boot
 * @return Milliseconds since boot
 */
osal_ms_t osal_get_time_ms(void);

/**
 * @brief Get time in microseconds since boot
 * @return Microseconds since boot
 */
osal_us_t osal_get_time_us(void);

/**
 * @brief Convert ticks to milliseconds
 * @param ticks Tick count
 * @return Milliseconds
 */
osal_ms_t osal_ticks_to_ms(osal_tick_t ticks);

/**
 * @brief Convert milliseconds to ticks
 * @param ms Milliseconds
 * @return Tick count
 */
osal_tick_t osal_ms_to_ticks(osal_ms_t ms);

/* ============================================================================
 * System Functions
 * ============================================================================ */

/**
 * @brief Initialize OSAL
 * @return OSAL_OK on success
 */
osal_err_t osal_init(void);

/**
 * @brief Start scheduler (if not auto-started)
 * @note On FreeRTOS/Zephyr this typically doesn't return
 */
void osal_start_scheduler(void);

/**
 * @brief Get free heap size
 * @return Free heap in bytes
 */
size_t osal_get_free_heap(void);

/**
 * @brief Get minimum free heap ever recorded
 * @return Minimum free heap in bytes
 */
size_t osal_get_min_free_heap(void);

/**
 * @brief Software reset
 */
void osal_system_reset(void);

/**
 * @brief Get current number of tasks/threads
 * @return Number of active tasks (0 on bare-metal)
 */
uint32_t osal_get_task_count(void);

/**
 * @brief Get cumulative idle task runtime in microseconds
 * @return Idle runtime in microseconds (0 if unsupported)
 */
uint32_t osal_get_idle_runtime_us(void);

#ifdef __cplusplus
}
#endif

/* ============================================================================
 * C++ Helpers
 * ============================================================================ */
#ifdef __cplusplus

namespace osal {

/**
 * @brief RAII mutex lock guard
 */
class MutexGuard {
public:
    explicit MutexGuard(osal_mutex_handle_t mutex, osal_ms_t timeout = OSAL_WAIT_FOREVER)
        : _mutex(mutex), _locked(false) {
        if (osal_mutex_lock(_mutex, timeout) == OSAL_OK) {
            _locked = true;
        }
    }
    
    ~MutexGuard() {
        if (_locked) {
            osal_mutex_unlock(_mutex);
        }
    }
    
    bool isLocked() const { return _locked; }
    
    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;
    
private:
    osal_mutex_handle_t _mutex;
    bool _locked;
};

/**
 * @brief RAII critical section guard
 */
class CriticalSection {
public:
    CriticalSection() : _state(osal_critical_enter()) {}
    ~CriticalSection() { osal_critical_exit(_state); }
    
    CriticalSection(const CriticalSection&) = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;
    
private:
    uint32_t _state;
};

} // namespace osal

#endif // __cplusplus

#endif /* OSAL_H */