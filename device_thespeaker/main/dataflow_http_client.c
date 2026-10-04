/**
 * HTTP Client for sending data to backend
 * Generated for theSpeaker
 */
#include "dataflow_http_client.h"
#include "pal/pal_http.h"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if __has_include("protocol_version.h")
#include "protocol_version.h"
#endif
#if __has_include("dataflow_heartbeat.h")
#include "dataflow_heartbeat.h"
#endif
#if __has_include("dataflow_alert.h")
#include "dataflow_alert.h"
#endif
#if __has_include("dataflow_ack.h")
#include "dataflow_ack.h"
#endif

OSAL_LOG_MODULE_REGISTER(dataflow_http)

/* Backend configuration */
static char s_backend_host[64] = BACKEND_HOST;
static int s_backend_port = BACKEND_PORT;
static char s_backend_url[128];
static uint32_t s_tx_seq = 0;
static uint32_t s_tx_count = 0;
static uint32_t s_tx_errors = 0;

// Data flow variables
uint8_t g_dataflow_thespeaker_bluetoothspeaker_connected = 0;
char* g_dataflow_thespeaker_bluetoothspeaker_device_name = 0;
int32_t g_dataflow_thespeaker_bluetoothspeaker_rssi = 0;

void dataflow_http_init(void)
{
    snprintf(s_backend_url, sizeof(s_backend_url), 
             "http://%s:%d%s", s_backend_host, s_backend_port, BACKEND_ENDPOINT);
    OSAL_LOGI("HTTP client initialized: %s", s_backend_url);
}

void dataflow_http_set_backend(const char* host, int port)
{
    strncpy(s_backend_host, host, sizeof(s_backend_host) - 1);
    s_backend_host[sizeof(s_backend_host) - 1] = '\0';
    s_backend_port = port;
    
    snprintf(s_backend_url, sizeof(s_backend_url), 
             "http://%s:%d%s", s_backend_host, s_backend_port, BACKEND_ENDPOINT);
    OSAL_LOGI("Backend URL updated: %s", s_backend_url);
}

bool dataflow_http_send(void)
{
    // Build JSON payload
    cJSON* root = cJSON_CreateObject();
    if (root == NULL) {
        OSAL_LOGE("Failed to create JSON object");
        return false;
    }
    
    /* Add MCU identifier and protocol version */
    cJSON_AddStringToObject(root, "mcu_id", "theSpeaker");
    cJSON_AddNumberToObject(root, "timestamp", (double)osal_get_time_ms());
    cJSON_AddNumberToObject(root, "_seq", (double)s_tx_seq);
#if __has_include("protocol_version.h")
    cJSON_AddStringToObject(root, "_protocol_version", PROTOCOL_VERSION_STRING);
#endif
    
    // Add data flow variables
    cJSON_AddBoolToObject(root, "thespeaker_bluetoothspeaker_connected", g_dataflow_thespeaker_bluetoothspeaker_connected);
    cJSON_AddStringToObject(root, "thespeaker_bluetoothspeaker_device_name", g_dataflow_thespeaker_bluetoothspeaker_device_name ? g_dataflow_thespeaker_bluetoothspeaker_device_name : "");
    cJSON_AddNumberToObject(root, "thespeaker_bluetoothspeaker_rssi", (double)g_dataflow_thespeaker_bluetoothspeaker_rssi);
    
    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (json_string == NULL) {
        OSAL_LOGE("Failed to serialize JSON");
        return false;
    }
    
    // Use PAL HTTP abstraction
    pal_http_response_t resp = {0};
    int ret = pal_http_post(s_backend_url, json_string, &resp);
    bool success = false;
    
    if (ret == 0) {
        if (resp.status_code >= 200 && resp.status_code < 300) {
            OSAL_LOGD("Data sent successfully (HTTP %d)", resp.status_code);
            s_tx_count++;
            s_tx_seq++;
            success = true;
        } else {
            s_tx_errors++;
            OSAL_LOGW("Backend returned HTTP %d", resp.status_code);
        }
    } else {
        s_tx_errors++;
        OSAL_LOGE("HTTP POST failed: err=%d", ret);
    }
    
    pal_http_free_response(&resp);
    free(json_string);
    
    return success;
}