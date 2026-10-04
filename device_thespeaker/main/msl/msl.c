/**
 * @file msl.c
 * @brief Message Subscription Layer - Core Implementation
 *
 * Manages subscription table, dispatches messages to handlers,
 * and coordinates protocol backend lifecycle.
 *
 * Auto-generated for project: theSpeaker (3 boards)
 */

#include "msl.h"
#include <string.h>


#include "osal/osal_log.h"
#include "osal/osal.h"
#include "osal/osal_sync.h"
#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#endif
OSAL_LOG_MODULE_REGISTER(msl)
#define MSL_LOGI(fmt, ...) OSAL_LOGI(fmt, ##__VA_ARGS__)
#define MSL_LOGW(fmt, ...) OSAL_LOGW(fmt, ##__VA_ARGS__)
#define MSL_LOGE(fmt, ...) OSAL_LOGE(fmt, ##__VA_ARGS__)


/* ─── Subscription Table ─── */

typedef struct {
    bool active;
    msl_descriptor_t descriptor;
    msl_handler_t handler;
    void *user_ctx;
} msl_subscription_t;

static msl_subscription_t s_subscriptions[MSL_MAX_SUBSCRIPTIONS];
static uint16_t s_sub_count = 0;

/* ─── Backend Registry ─── */

static const msl_backend_ops_t *s_backends[MSL_MAX_PROTOCOLS];
static uint8_t s_backend_count = 0;

/* ─── Error & State Handlers ─── */

static msl_error_handler_t s_error_handlers[MSL_PROTO_COUNT + 1];
static msl_state_handler_t s_state_handlers[MSL_PROTO_COUNT + 1];

/* ─── Synchronization ─── */


static SemaphoreHandle_t s_msl_mutex = NULL;
#define MSL_LOCK()   do { if (s_msl_mutex) xSemaphoreTake(s_msl_mutex, portMAX_DELAY); } while(0)
#define MSL_UNLOCK() do { if (s_msl_mutex) xSemaphoreGive(s_msl_mutex); } while(0)


/* ─── Helper: Find backend for protocol ─── */

static const msl_backend_ops_t *find_backend(msl_protocol_t protocol)
{
    for (uint8_t i = 0; i < s_backend_count; i++) {
        if (s_backends[i] && s_backends[i]->protocol == protocol) {
            return s_backends[i];
        }
    }
    return NULL;
}

/* ─── Public API Implementation ─── */

void msl_init(void)
{
    memset(s_subscriptions, 0, sizeof(s_subscriptions));
    memset(s_backends, 0, sizeof(s_backends));
    memset(s_error_handlers, 0, sizeof(s_error_handlers));
    memset(s_state_handlers, 0, sizeof(s_state_handlers));
    s_sub_count = 0;
    s_backend_count = 0;


    s_msl_mutex = xSemaphoreCreateMutex();


    MSL_LOGI("MSL initialized (max_subs=%d)", MSL_MAX_SUBSCRIPTIONS);
}

msl_sub_handle_t msl_subscribe(const msl_descriptor_t *desc, msl_handler_t handler, void *user_ctx)
{
    if (!desc || !handler) return MSL_INVALID_HANDLE;

    MSL_LOCK();

    /* Find free slot */
    uint16_t slot = MSL_INVALID_HANDLE;
    for (uint16_t i = 0; i < MSL_MAX_SUBSCRIPTIONS; i++) {
        if (!s_subscriptions[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == MSL_INVALID_HANDLE) {
        MSL_UNLOCK();
        MSL_LOGE("No free subscription slots");
        return MSL_INVALID_HANDLE;
    }

    s_subscriptions[slot].active = true;
    s_subscriptions[slot].descriptor = *desc;
    s_subscriptions[slot].handler = handler;
    s_subscriptions[slot].user_ctx = user_ctx;
    s_sub_count++;

    /* Notify backend of new subscription */
    const msl_backend_ops_t *backend = find_backend(desc->protocol);
    if (backend && backend->subscribe) {
        int ret = backend->subscribe(desc, slot);
        if (ret != 0) {
            MSL_LOGW("Backend subscribe failed for proto=%d, slot=%d, err=%d",
                     desc->protocol, slot, ret);
        }
    }

    MSL_UNLOCK();

    MSL_LOGI("Subscribed: proto=%s, handle=%d",
             msl_protocol_name(desc->protocol), slot);
    return slot;
}

bool msl_unsubscribe(msl_sub_handle_t handle)
{
    if (handle >= MSL_MAX_SUBSCRIPTIONS) return false;

    MSL_LOCK();

    if (!s_subscriptions[handle].active) {
        MSL_UNLOCK();
        return false;
    }

    /* Notify backend */
    const msl_backend_ops_t *backend = find_backend(s_subscriptions[handle].descriptor.protocol);
    if (backend && backend->unsubscribe) {
        backend->unsubscribe(handle);
    }

    s_subscriptions[handle].active = false;
    s_subscriptions[handle].handler = NULL;
    s_subscriptions[handle].user_ctx = NULL;
    s_sub_count--;

    MSL_UNLOCK();
    return true;
}

int msl_publish(const msl_descriptor_t *desc, const uint8_t *data, size_t length)
{
    if (!desc) return -1;

    const msl_backend_ops_t *backend = find_backend(desc->protocol);
    if (!backend) {
        MSL_LOGE("No backend registered for proto=%d", desc->protocol);
        return -1;
    }

    if (!backend->publish) {
        MSL_LOGE("Backend %s does not support publish", backend->name);
        return -1;
    }

    msl_state_t state = backend->get_state ? backend->get_state() : MSL_STATE_OPERATIONAL;
    if (state != MSL_STATE_OPERATIONAL) {
        MSL_LOGW("Cannot publish: %s in state %s",
                 backend->name, msl_state_name(state));
        return -2;
    }

    return backend->publish(desc, data, length);
}

msl_state_t msl_get_state(msl_protocol_t protocol)
{
    const msl_backend_ops_t *backend = find_backend(protocol);
    if (!backend || !backend->get_state) return MSL_STATE_IDLE;
    return backend->get_state();
}

void msl_on_error(msl_protocol_t protocol, msl_error_handler_t handler)
{
    if (protocol <= MSL_PROTO_COUNT) {
        s_error_handlers[protocol] = handler;
    }
}

void msl_on_state_change(msl_protocol_t protocol, msl_state_handler_t handler)
{
    if (protocol <= MSL_PROTO_COUNT) {
        s_state_handlers[protocol] = handler;
    }
}

void msl_process(void)
{
    for (uint8_t i = 0; i < s_backend_count; i++) {
        if (s_backends[i] && s_backends[i]->process) {
            s_backends[i]->process();
        }
    }
}

void msl_shutdown(void)
{
    MSL_LOGI("Shutting down MSL");

    for (uint8_t i = 0; i < s_backend_count; i++) {
        if (s_backends[i] && s_backends[i]->deinit) {
            MSL_LOGI("Deinit backend: %s", s_backends[i]->name);
            s_backends[i]->deinit();
        }
    }

    MSL_LOCK();
    memset(s_subscriptions, 0, sizeof(s_subscriptions));
    s_sub_count = 0;
    s_backend_count = 0;
    MSL_UNLOCK();


    if (s_msl_mutex) {
        vSemaphoreDelete(s_msl_mutex);
        s_msl_mutex = NULL;
    }

}

int msl_register_backend(const msl_backend_ops_t *ops)
{
    if (!ops) return -1;
    if (s_backend_count >= MSL_MAX_PROTOCOLS) {
        MSL_LOGE("Max backends reached (%d)", MSL_MAX_PROTOCOLS);
        return -1;
    }

    /* Check for duplicate */
    for (uint8_t i = 0; i < s_backend_count; i++) {
        if (s_backends[i] && s_backends[i]->protocol == ops->protocol) {
            MSL_LOGW("Backend already registered for proto=%d, replacing", ops->protocol);
            s_backends[i] = ops;
            return 0;
        }
    }

    s_backends[s_backend_count++] = ops;
    MSL_LOGI("Registered backend: %s (proto=%d)", ops->name, ops->protocol);

    /* Initialize the backend */
    if (ops->init) {
        int ret = ops->init();
        if (ret != 0) {
            MSL_LOGE("Backend init failed: %s, err=%d", ops->name, ret);
            return ret;
        }
    }

    return 0;
}

/* ─── Dispatch function (called by backends when message arrives) ─── */

void msl_dispatch(const msl_message_t *msg)
{
    if (!msg) return;

    MSL_LOCK();
    for (uint16_t i = 0; i < MSL_MAX_SUBSCRIPTIONS; i++) {
        if (!s_subscriptions[i].active) continue;
        if (s_subscriptions[i].descriptor.protocol != msg->protocol) continue;

        /* Protocol-specific matching is handled by the backend before dispatch,
         * but we do a basic protocol match here as safety. The backend's
         * process() function should only call msl_dispatch for messages that
         * already passed its internal filtering. */
        s_subscriptions[i].handler(msg, s_subscriptions[i].user_ctx);
    }
    MSL_UNLOCK();
}

/* ─── Utility functions ─── */

const char *msl_protocol_name(msl_protocol_t protocol)
{
    static const char *names[] = {
        [MSL_PROTO_NONE]        = "none",
        [MSL_PROTO_CAN]         = "CAN",
        [MSL_PROTO_CANOPEN]     = "CANopen",
        [MSL_PROTO_MQTT]        = "MQTT",
        [MSL_PROTO_BLE]         = "BLE",
        [MSL_PROTO_MODBUS_RTU]  = "Modbus RTU",
        [MSL_PROTO_MODBUS_TCP]  = "Modbus TCP",
        [MSL_PROTO_HTTP]        = "HTTP",
        [MSL_PROTO_WEBSOCKET]   = "WebSocket",
        [MSL_PROTO_COAP]        = "CoAP",
        [MSL_PROTO_LORA]        = "LoRa",
        [MSL_PROTO_LORAWAN]     = "LoRaWAN",
        [MSL_PROTO_ZIGBEE]      = "Zigbee",
        [MSL_PROTO_THREAD]      = "Thread",
        [MSL_PROTO_MATTER]      = "Matter",
        [MSL_PROTO_UART]        = "UART",
        [MSL_PROTO_SPI]         = "SPI",
        [MSL_PROTO_I2C]         = "I2C",
        [MSL_PROTO_ESPNOW]      = "ESP-NOW",
        [MSL_PROTO_UDP]         = "UDP",
        [MSL_PROTO_TCP]         = "TCP",
        [MSL_PROTO_ONEWIRE]     = "1-Wire",
        [MSL_PROTO_I2S]         = "I2S",
        [MSL_PROTO_NFC]         = "NFC",
        [MSL_PROTO_UWB]         = "UWB",
        [MSL_PROTO_SIGFOX]      = "Sigfox",
        [MSL_PROTO_NBIOT]       = "NB-IoT",
        [MSL_PROTO_LTEM]        = "LTE-M",
        [MSL_PROTO_ETHERNET]    = "Ethernet",
        [MSL_PROTO_OPCUA]       = "OPC-UA",
        [MSL_PROTO_ETHERCAT]    = "EtherCAT",
        [MSL_PROTO_PROFINET]    = "PROFINET",
        [MSL_PROTO_GRPC]        = "gRPC",
        [MSL_PROTO_AMQP]        = "AMQP",
    };
    if (protocol < MSL_PROTO_COUNT) return names[protocol];
    return "unknown";
}

const char *msl_state_name(msl_state_t state)
{
    static const char *names[] = {
        [MSL_STATE_IDLE]         = "IDLE",
        [MSL_STATE_BRINGING_UP]  = "BRINGING_UP",
        [MSL_STATE_OPERATIONAL]  = "OPERATIONAL",
        [MSL_STATE_ERROR]        = "ERROR",
        [MSL_STATE_RECONNECTING] = "RECONNECTING",
        [MSL_STATE_SHUTDOWN]     = "SHUTDOWN",
    };
    if (state <= MSL_STATE_SHUTDOWN) return names[state];
    return "unknown";
}