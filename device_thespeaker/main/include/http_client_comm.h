/**
 * @file http_client_comm.h
 * @brief HTTP Client Communication
 * 
 * HTTP Client that:
 * - SENDS: Sensor data via POST to REST API
 * - RECEIVES: Configuration via REST (commands pushed via WebSocket channel)
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#ifndef HTTP_CLIENT_COMM_H
#define HTTP_CLIENT_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "osal/osal_types.h"
#include "http_comm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi Credentials - define these in your sdkconfig or project config */
#ifndef HTTP_WIFI_SSID
#define HTTP_WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef HTTP_WIFI_PASS
#define HTTP_WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif

/* Configuration */
#define HTTP_SERVER_URL             "http://192.168.1.100:8000"
#define HTTP_API_KEY                ""
#define HTTP_DEVICE_ID              "esp32-001"
#define HTTP_DEFAULT_INTERVAL       1000
#define HTTP_USE_HTTPS              false

/* HTTP client callback type */
typedef void (*http_client_callback_t)(const char *data, size_t len, int status_code);

#define HTTP_SENSOR_COUNT           4

/**
 * @brief Callback for connection state
 */
typedef void (*http_client_conn_callback_t)(bool connected);

/**
 * @brief Initialize HTTP client
 */
esp_err_t http_client_init(void);

/**
 * @brief Deinitialize HTTP client
 */
esp_err_t http_client_deinit(void);

/**
 * @brief Set callback for connection state
 */
void http_client_set_conn_callback(http_client_conn_callback_t callback);

/**
 * @brief Set API key
 */
esp_err_t http_client_set_api_key(const char *key);

/* ═══════════════════════════════════════════════════════════════════
 * DATA SENDING
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t http_client_post_sensor(uint8_t sensor_id, http_sensor_type_t type, float value);
esp_err_t http_client_post_all_sensors(const http_sensor_data_t *sensors, size_t count);
esp_err_t http_client_get(const char *url, char *response, size_t max_len, int *status_code);
esp_err_t http_client_post(const char *url, const char *data, char *response, size_t max_len, int *status_code);
struct cJSON;  /* Forward declaration */
esp_err_t http_client_post_json(const char *url, const struct cJSON *json, char *response, size_t max_len, int *status_code);

/* Convenience functions */
esp_err_t http_client_post_xpt2046_touch_controller(float value);
esp_err_t http_client_post_ldr_light_sensor(float value);
esp_err_t http_client_post_microsd_card_slot(float value);
esp_err_t http_client_post_speaker_connector_8002a_amp(float value);

/* ═══════════════════════════════════════════════════════════════════
 * DATA RECEIVING
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t http_client_get_config(void);
esp_err_t http_client_get_json(const char *endpoint, char *buffer, size_t buffer_len);

/* ═══════════════════════════════════════════════════════════════════
 * UTILITIES
 * ═══════════════════════════════════════════════════════════════════ */

uint32_t http_client_get_interval(void);
void http_client_set_interval(uint32_t interval_ms);
bool http_client_is_connected(void);
void http_client_get_stats(uint32_t *requests, uint32_t *successes, uint32_t *errors);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_COMM_H