/**
 * @file osal_config.h
 * @brief OS Abstraction Layer Configuration

 * 
 * Configures which OS backend to use and common settings.
 */

#ifndef OSAL_CONFIG_H
#define OSAL_CONFIG_H

/* OS Selection - set by build system or here */
#define OSAL_USE_FREERTOS       1
#define OSAL_USE_ZEPHYR         0
#define OSAL_USE_LINUX          0
#define OSAL_USE_BAREMETAL      0

/* Common configuration */
#define OSAL_MAX_TASK_NAME_LEN  32
#define OSAL_DEFAULT_STACK_SIZE 4096
#define OSAL_DEFAULT_PRIORITY   5
#define OSAL_TICK_RATE_HZ       1000

/* Feature flags */
#define OSAL_ENABLE_STATS       0
#define OSAL_ENABLE_TRACE       0

#endif /* OSAL_CONFIG_H */