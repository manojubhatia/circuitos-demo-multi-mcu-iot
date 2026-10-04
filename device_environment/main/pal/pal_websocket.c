/**
 * @file pal_websocket.c
 * @brief PAL WebSocket implementation for ESP-IDF
 */
#include "pal_websocket.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "pal_ws";
static esp_websocket_client_handle_t s_client = NULL;
static pal_ws_msg_cb_t s_msg_cb = NULL;
static void *s_msg_ud = NULL;
static bool s_connected = false;

static void ws_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_websocket_event_data_t *evt = (esp_websocket_event_data_t *)data;
    switch (id) {
        case WEBSOCKET_EVENT_CONNECTED: s_connected = true; ESP_LOGI(TAG, "WS connected"); break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "WS disconnected — auto-reconnect will retry");
            break;
        case WEBSOCKET_EVENT_DATA:
            if (s_msg_cb && evt->data_len > 0)
                s_msg_cb((uint8_t *)evt->data_ptr, evt->data_len, evt->op_code == 2, s_msg_ud);
            break;
        default: break;
    }
}

int pal_ws_init(const pal_ws_config_t *config)
{
    esp_websocket_client_config_t cfg = {
        .uri = config->uri,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
        .ping_interval_sec = 10,
    };
    s_client = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
    ESP_LOGI(TAG, "WS init: %s (auto-reconnect 5s)", config->uri);
    return 0;
}

int pal_ws_connect(void)
{
    if (esp_websocket_client_is_connected(s_client))
        return 0;
    return (esp_websocket_client_start(s_client) == ESP_OK) ? 0 : -1;
}
void pal_ws_disconnect(void) { esp_websocket_client_stop(s_client); s_connected = false; }

int pal_ws_send_text(const char *data, size_t len)
{
    int sent = esp_websocket_client_send_text(s_client, data, (int)len, portMAX_DELAY);
    return (sent >= 0) ? 0 : -1;
}

int pal_ws_send_binary(const uint8_t *data, size_t len)
{
    int sent = esp_websocket_client_send_bin(s_client, (const char *)data, (int)len, portMAX_DELAY);
    return (sent >= 0) ? 0 : -1;
}

void pal_ws_on_message(pal_ws_msg_cb_t cb, void *ud) { s_msg_cb = cb; s_msg_ud = ud; }
void pal_ws_on_event(pal_ws_event_cb_t cb, void *ud) { (void)cb; (void)ud; }
bool pal_ws_is_connected(void) { return s_connected; }