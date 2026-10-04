/**
 * @file osal_log.c
 * @brief OS Abstraction Layer - Logging Implementation

 */

#include "osal/osal_log.h"
#include <stdarg.h>
#include <stdio.h>

/* ── ESP-IDF backend ─────────────────────────────────────────────── */
#include "osal/osal_log.h"
#include "esp_log.h"

void osal_log_write(osal_log_level_t level, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    esp_log_level_t esp_level;
    switch (level) {
        case OSAL_LOG_ERROR:   esp_level = ESP_LOG_ERROR;   break;
        case OSAL_LOG_WARN:    esp_level = ESP_LOG_WARN;    break;
        case OSAL_LOG_INFO:    esp_level = ESP_LOG_INFO;    break;
        case OSAL_LOG_DEBUG:   esp_level = ESP_LOG_DEBUG;   break;
        case OSAL_LOG_VERBOSE: esp_level = ESP_LOG_VERBOSE; break;
        default:               esp_level = ESP_LOG_INFO;    break;
    }
    esp_log_writev(esp_level, tag, fmt, args);
    va_end(args);
}

void osal_log_set_level(const char *tag, osal_log_level_t level) {
    esp_log_level_t esp_level;
    switch (level) {
        case OSAL_LOG_NONE:    esp_level = ESP_LOG_NONE;    break;
        case OSAL_LOG_ERROR:   esp_level = ESP_LOG_ERROR;   break;
        case OSAL_LOG_WARN:    esp_level = ESP_LOG_WARN;    break;
        case OSAL_LOG_INFO:    esp_level = ESP_LOG_INFO;    break;
        case OSAL_LOG_DEBUG:   esp_level = ESP_LOG_DEBUG;   break;
        case OSAL_LOG_VERBOSE: esp_level = ESP_LOG_VERBOSE; break;
        default:               esp_level = ESP_LOG_INFO;    break;
    }
    esp_log_level_set(tag, esp_level);
}

