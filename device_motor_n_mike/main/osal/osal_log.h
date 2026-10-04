/**
 * @file osal_log.h
 * @brief OS Abstraction Layer - Unified Logging

 *
 * Provides a portable logging API that works across:
 * - ESP-IDF (wraps esp_log)
 * - Zephyr RTOS (wraps Zephyr logging)
 * - Linux/POSIX (wraps fprintf)
 * - Bare-metal / Mock (printf)
 */

#ifndef OSAL_LOG_H
#define OSAL_LOG_H

#include "osal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OSAL_LOG_NONE    = 0,
    OSAL_LOG_ERROR   = 1,
    OSAL_LOG_WARN    = 2,
    OSAL_LOG_INFO    = 3,
    OSAL_LOG_DEBUG   = 4,
    OSAL_LOG_VERBOSE = 5
} osal_log_level_t;

/**
 * @brief Register a log tag for the current module.
 * Place once at the top of each .c file, after includes.
 */
#define OSAL_LOG_MODULE_REGISTER(tag) \
    static const char *_osal_log_tag __attribute__((unused)) = #tag;

/* Short-form macros — use the module tag registered via OSAL_LOG_MODULE_REGISTER */
#define OSAL_LOGE(fmt, ...) osal_log_write(OSAL_LOG_ERROR,   _osal_log_tag, fmt, ##__VA_ARGS__)
#define OSAL_LOGW(fmt, ...) osal_log_write(OSAL_LOG_WARN,    _osal_log_tag, fmt, ##__VA_ARGS__)
#define OSAL_LOGI(fmt, ...) osal_log_write(OSAL_LOG_INFO,    _osal_log_tag, fmt, ##__VA_ARGS__)
#define OSAL_LOGD(fmt, ...) osal_log_write(OSAL_LOG_DEBUG,   _osal_log_tag, fmt, ##__VA_ARGS__)
#define OSAL_LOGV(fmt, ...) osal_log_write(OSAL_LOG_VERBOSE, _osal_log_tag, fmt, ##__VA_ARGS__)

/* Explicit-tag macros — used by engine-generated code that specifies its own tag */
#define OSAL_LOG_ERROR(tag, fmt, ...)   osal_log_write(OSAL_LOG_ERROR,   tag, fmt, ##__VA_ARGS__)
#define OSAL_LOG_WARN(tag, fmt, ...)    osal_log_write(OSAL_LOG_WARN,    tag, fmt, ##__VA_ARGS__)
#define OSAL_LOG_INFO(tag, fmt, ...)    osal_log_write(OSAL_LOG_INFO,    tag, fmt, ##__VA_ARGS__)
#define OSAL_LOG_DEBUG(tag, fmt, ...)   osal_log_write(OSAL_LOG_DEBUG,   tag, fmt, ##__VA_ARGS__)
#define OSAL_LOG_VERBOSE(tag, fmt, ...) osal_log_write(OSAL_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__)

void osal_log_write(osal_log_level_t level, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

void osal_log_set_level(const char *tag, osal_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_LOG_H */