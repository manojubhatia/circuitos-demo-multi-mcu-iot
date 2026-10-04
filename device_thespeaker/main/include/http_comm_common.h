/**
 * @file http_comm_common.h
 * @brief Common HTTP Communication Definitions
 * 
 * HTTP/HTTPS client for REST API communication over WiFi.
 * Supports GET, POST, PUT, DELETE methods with JSON payloads.
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#ifndef HTTP_COMM_COMMON_H
#define HTTP_COMM_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HTTP Configuration */
#define HTTP_MAX_URL_LEN            256
#define HTTP_MAX_HEADER_LEN         512
#define HTTP_MAX_BODY_LEN           4096
#define HTTP_MAX_SENSORS            16
#define HTTP_DEFAULT_TIMEOUT_MS     10000

/* HTTP Methods - Use ESP-IDF's types when building for ESP32 */
#ifdef ESP_PLATFORM
#include "pal/pal_http.h"
/* esp_http_client.h defines HTTP_METHOD_* - use those directly */
#else
/* Non-ESP platforms: define our own */
typedef enum {
    HTTP_METHOD_GET     = 0,
    HTTP_METHOD_POST    = 1,
    HTTP_METHOD_PUT     = 2,
    HTTP_METHOD_DELETE  = 3,
    HTTP_METHOD_PATCH   = 4
} http_method_t;
#endif

/* HTTP query parameter */
typedef struct {
    const char *name;
    const char *value;
} http_param_t;

/* Protocol Status */
typedef enum {
    HTTP_OK                 = 0,
    HTTP_ERR_PARAM          = -1,
    HTTP_ERR_NO_WIFI        = -2,
    HTTP_ERR_DNS            = -3,
    HTTP_ERR_CONNECT        = -4,
    HTTP_ERR_TIMEOUT        = -5,
    HTTP_ERR_SSL            = -6,
    HTTP_ERR_RESPONSE       = -7
} http_status_t;

/* Command Types (from server) */
typedef enum {
    HTTP_CMD_SET_INTERVAL       = 0x01,
    HTTP_CMD_ENABLE_SENSOR      = 0x02,
    HTTP_CMD_SET_CONFIG         = 0x03,
    HTTP_CMD_REBOOT             = 0x04,
    HTTP_CMD_OTA_UPDATE         = 0x05,
    HTTP_CMD_SET_OUTPUT         = 0x10,
} http_cmd_type_t;

/* Sensor Type Identifiers */
typedef enum {
    HTTP_SENSOR_TYPE_UNKNOWN        = 0x00,
    HTTP_SENSOR_TYPE_TEMPERATURE    = 0x01,
    HTTP_SENSOR_TYPE_HUMIDITY       = 0x02,
    HTTP_SENSOR_TYPE_PRESSURE       = 0x03,
    HTTP_SENSOR_TYPE_LIGHT          = 0x04,
    HTTP_SENSOR_TYPE_MOTION         = 0x05,
    HTTP_SENSOR_TYPE_BATTERY        = 0x06,
    HTTP_SENSOR_TYPE_GENERIC        = 0xFF
} http_sensor_type_t;

/* Project-specific sensor IDs */
#define HTTP_SENSOR_ID_XPT2046_TOUCH_CONTROLLER    1
#define HTTP_SENSOR_ID_LDR_LIGHT_SENSOR    2
#define HTTP_SENSOR_ID_MICROSD_CARD_SLOT    3
#define HTTP_SENSOR_ID_SPEAKER_CONNECTOR_8002A_AMP    4

/**
 * @brief HTTP response
 */
typedef struct {
    int status_code;
    char *body;
    size_t body_len;
    char *content_type;
} http_response_t;

/**
 * @brief Sensor data for JSON
 */
typedef struct {
    uint8_t sensor_id;
    uint8_t sensor_type;
    float value;
    uint32_t timestamp_ms;
    const char *unit;
} http_sensor_data_t;

/**
 * @brief Server command
 */
typedef struct {
    http_cmd_type_t cmd_type;
    uint8_t target_id;
    uint32_t value;
    char *payload;
} http_server_cmd_t;

#ifdef __cplusplus
}
#endif

#endif // HTTP_COMM_COMMON_H