/**
 * @file http_client_comm.c
 * @brief HTTP Client Communication Implementation

 *
 * Uses OSAL/PAL abstractions for portability across platforms.
 */

#include "http_client_comm.h"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_sync.h"
#include "pal/pal_http.h"
#include "pal/pal_wifi.h"
#include "pal/pal_storage.h"
#include "cJSON.h"
#include <string.h>

OSAL_LOG_MODULE_REGISTER(http_client)

#define HTTP_MAX_OUTPUT_BUFFER 2048
#define HTTP_TIMEOUT_MS 10000

static osal_mutex_handle_t s_mutex = NULL;
static http_client_callback_t s_callback = NULL;
static bool s_initialized = false;

static void wifi_event_callback(pal_wifi_event_t event, void *user_data)
{
    (void)user_data;
    switch (event) {
        case PAL_WIFI_EVENT_CONNECTED:
            OSAL_LOGI("WiFi connected");
            break;
        case PAL_WIFI_EVENT_GOT_IP: {
            char ip_buf[16] = {0};
            pal_wifi_get_ip(ip_buf, sizeof(ip_buf));
            OSAL_LOGI("Got IP: %s", ip_buf);
            break;
        }
        case PAL_WIFI_EVENT_DISCONNECTED:
            OSAL_LOGI("WiFi disconnected, reconnecting...");
            {
                pal_wifi_config_t cfg = {
                    .ssid = HTTP_WIFI_SSID,
                    .password = HTTP_WIFI_PASS,
                    .max_retries = 5,
                    .timeout_ms = 10000,
                };
                pal_wifi_connect(&cfg);
            }
            break;
        case PAL_WIFI_EVENT_LOST_IP:
            OSAL_LOGW("Lost IP address");
            break;
    }
}

esp_err_t http_client_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    /* Initialize persistent storage (NVS) */
    if (pal_storage_init() != 0) {
        OSAL_LOGW("Storage init returned non-zero (may already be initialized)");
    }

    /* Initialize WiFi subsystem */
    if (pal_wifi_init() != 0) {
        OSAL_LOGE("WiFi init failed");
        return ESP_FAIL;
    }

    /* Register WiFi event callback */
    pal_wifi_set_callback(wifi_event_callback, NULL);

    /* Connect to WiFi */
    pal_wifi_config_t wifi_cfg = {
        .ssid = HTTP_WIFI_SSID,
        .password = HTTP_WIFI_PASS,
        .max_retries = 5,
        .timeout_ms = 15000,
    };
    if (pal_wifi_connect(&wifi_cfg) != 0) {
        OSAL_LOGE("WiFi connect failed");
        return ESP_FAIL;
    }

    /* Create mutex for thread safety */
    if (osal_mutex_create(&s_mutex) != OSAL_OK) {
        OSAL_LOGE("Failed to create mutex");
        return ESP_FAIL;
    }

    s_initialized = true;
    OSAL_LOGI("HTTP client initialized, connecting to WiFi...");
    return ESP_OK;
}

esp_err_t http_client_deinit(void)
{
    if (!s_initialized) {
        return ESP_OK;
    }

    pal_wifi_disconnect();

    if (s_mutex) {
        osal_mutex_delete(s_mutex);
        s_mutex = NULL;
    }

    s_initialized = false;
    return ESP_OK;
}

void http_client_set_callback(http_client_callback_t callback)
{
    s_callback = callback;
}

bool http_client_is_connected(void)
{
    return pal_wifi_is_connected();
}

esp_err_t http_client_get(const char *url, char *response, size_t max_len, int *status_code)
{
    if (!pal_wifi_is_connected()) {
        return ESP_FAIL;
    }

    osal_mutex_lock(s_mutex, OSAL_WAIT_FOREVER);

    pal_http_response_t resp = {0};
    int rc = pal_http_get(url, &resp);

    if (rc == 0) {
        if (status_code) {
            *status_code = resp.status_code;
        }
        if (response && max_len > 0 && resp.body) {
            size_t copy_len = (resp.body_len < max_len - 1) ? resp.body_len : max_len - 1;
            memcpy(response, resp.body, copy_len);
            response[copy_len] = '\0';
        }
        if (s_callback && resp.body) {
            s_callback(resp.body, resp.body_len, resp.status_code);
        }
        pal_http_free_response(&resp);
    }

    osal_mutex_unlock(s_mutex);
    return (rc == 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t http_client_post(const char *url, const char *data, char *response, size_t max_len, int *status_code)
{
    if (!pal_wifi_is_connected()) {
        return ESP_FAIL;
    }

    osal_mutex_lock(s_mutex, OSAL_WAIT_FOREVER);

    pal_http_response_t resp = {0};
    int rc = pal_http_post(url, data, &resp);

    if (rc == 0) {
        if (status_code) {
            *status_code = resp.status_code;
        }
        if (response && max_len > 0 && resp.body) {
            size_t copy_len = (resp.body_len < max_len - 1) ? resp.body_len : max_len - 1;
            memcpy(response, resp.body, copy_len);
            response[copy_len] = '\0';
        }
        if (s_callback && resp.body) {
            s_callback(resp.body, resp.body_len, resp.status_code);
        }
        pal_http_free_response(&resp);
    }

    osal_mutex_unlock(s_mutex);
    return (rc == 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t http_client_post_json(const char *url, const cJSON *json, char *response, size_t max_len, int *status_code)
{
    char *json_str = cJSON_PrintUnformatted(json);
    if (!json_str) {
        return ESP_FAIL;
    }

    esp_err_t ret = http_client_post(url, json_str, response, max_len, status_code);
    free(json_str);
    return ret;
}

esp_err_t http_client_send_data()
{
    char url[256];
    snprintf(url, sizeof(url), "%s/api/mcu/board_1/data", HTTP_SERVER_URL);
    
    return http_client_post(url, "{}", NULL, 0, NULL);
}
