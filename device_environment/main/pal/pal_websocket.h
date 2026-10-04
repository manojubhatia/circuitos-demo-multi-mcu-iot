/**
 * @file pal_websocket.h
 * @brief Protocol Abstraction Layer - WebSocket Client
 */
#ifndef PAL_WEBSOCKET_H
#define PAL_WEBSOCKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*pal_ws_msg_cb_t)(const uint8_t *data, size_t len, bool is_binary, void *user_data);
typedef void (*pal_ws_event_cb_t)(int event, void *user_data);

typedef struct {
    const char *uri;
    const char *subprotocol;
    uint32_t ping_interval_ms;
} pal_ws_config_t;

int pal_ws_init(const pal_ws_config_t *config);
int pal_ws_connect(void);
void pal_ws_disconnect(void);
int pal_ws_send_text(const char *data, size_t len);
int pal_ws_send_binary(const uint8_t *data, size_t len);
void pal_ws_on_message(pal_ws_msg_cb_t cb, void *user_data);
void pal_ws_on_event(pal_ws_event_cb_t cb, void *user_data);
bool pal_ws_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* PAL_WEBSOCKET_H */