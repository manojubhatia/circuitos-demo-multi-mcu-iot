/**
 * @file osal.c
 * @brief OS Abstraction Layer - System Functions Implementation

 * @platform esp_idf
 */

#include "osal.h"

/* ============================================================================
 * FreeRTOS/ESP-IDF Implementation
 * ============================================================================ */

#include "osal/osal.h"
#include "osal/osal_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

osal_tick_t osal_get_tick_count(void) {
    return xTaskGetTickCount();
}

osal_ms_t osal_get_time_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

osal_us_t osal_get_time_us(void) {
    return (uint64_t)xTaskGetTickCount() * portTICK_PERIOD_MS * 1000;
}

osal_ms_t osal_ticks_to_ms(osal_tick_t ticks) {
    return ticks * portTICK_PERIOD_MS;
}

osal_tick_t osal_ms_to_ticks(osal_ms_t ms) {
    return pdMS_TO_TICKS(ms);
}

osal_err_t osal_init(void) {
    return OSAL_OK;
}

void osal_start_scheduler(void) {
    // In ESP-IDF, the FreeRTOS scheduler is already running when app_main() is called.
    // Calling vTaskStartScheduler() again would cause an abort.
    // This is intentionally a no-op for ESP-IDF.
}

size_t osal_get_free_heap(void) {
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

size_t osal_get_min_free_heap(void) {
    return heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
}

void osal_system_reset(void) {
    esp_restart();
}

uint32_t osal_get_task_count(void) {
    return (uint32_t)uxTaskGetNumberOfTasks();
}

uint32_t osal_get_idle_runtime_us(void) {
#if defined(configGENERATE_RUN_TIME_STATS) && (configGENERATE_RUN_TIME_STATS == 1)
    return ulTaskGetIdleRunTimeCounter();
#else
    return 0;
#endif
}

