/**
 * @file msl_types.h
 * @brief Message Subscription Layer - Type Definitions
 *
 * Protocol descriptor unions, payload types, and callback signatures
 * used by all MSL backends.
 *
 * Auto-generated for project: theSpeaker (3 boards)
 */

#ifndef MSL_TYPES_H
#define MSL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum subscriptions per node */
#define MSL_MAX_SUBSCRIPTIONS       32
#define MSL_MAX_PAYLOAD_SIZE        256
#define MSL_MAX_TOPIC_LEN           128
#define MSL_MAX_PROTOCOLS           8

/* Protocol identifiers */
typedef enum {
    MSL_PROTO_NONE = 0,
    MSL_PROTO_CAN,
    MSL_PROTO_CANOPEN,
    MSL_PROTO_MQTT,
    MSL_PROTO_BLE,
    MSL_PROTO_MODBUS_RTU,
    MSL_PROTO_MODBUS_TCP,
    MSL_PROTO_HTTP,
    MSL_PROTO_WEBSOCKET,
    MSL_PROTO_COAP,
    MSL_PROTO_LORA,
    MSL_PROTO_LORAWAN,
    MSL_PROTO_ZIGBEE,
    MSL_PROTO_THREAD,
    MSL_PROTO_MATTER,
    MSL_PROTO_UART,
    MSL_PROTO_SPI,
    MSL_PROTO_I2C,
    MSL_PROTO_ESPNOW,
    MSL_PROTO_UDP,
    MSL_PROTO_TCP,
    MSL_PROTO_ONEWIRE,
    MSL_PROTO_I2S,
    MSL_PROTO_NFC,
    MSL_PROTO_UWB,
    MSL_PROTO_SIGFOX,
    MSL_PROTO_NBIOT,
    MSL_PROTO_LTEM,
    MSL_PROTO_ETHERNET,
    MSL_PROTO_OPCUA,
    MSL_PROTO_ETHERCAT,
    MSL_PROTO_PROFINET,
    MSL_PROTO_GRPC,
    MSL_PROTO_AMQP,
    MSL_PROTO_COUNT
} msl_protocol_t;

/* Lifecycle states (generic FSM for all protocols) */
typedef enum {
    MSL_STATE_IDLE = 0,
    MSL_STATE_BRINGING_UP,
    MSL_STATE_OPERATIONAL,
    MSL_STATE_ERROR,
    MSL_STATE_RECONNECTING,
    MSL_STATE_SHUTDOWN
} msl_state_t;

/* Message planes */
typedef enum {
    MSL_PLANE_MANAGEMENT = 0,
    MSL_PLANE_SERVICE,
    MSL_PLANE_DATA
} msl_plane_t;

/* Protocol-specific descriptor fields (union) */
typedef struct {
    uint32_t id;            /* CAN ID (11-bit or 29-bit) */
    bool extended;          /* true = 29-bit extended frame */
    uint32_t mask;          /* Acceptance mask for HW filter */
} msl_can_descriptor_t;

typedef struct {
    uint16_t node_id;       /* CANopen node ID (1-127) */
    uint16_t index;         /* Object Dictionary index */
    uint8_t subindex;       /* Object Dictionary subindex */
    bool is_pdo;            /* true = PDO, false = SDO */
    uint8_t pdo_num;        /* PDO number (1-4 for TPDO/RPDO) */
} msl_canopen_descriptor_t;

typedef struct {
    char topic[MSL_MAX_TOPIC_LEN];  /* MQTT topic (supports wildcards +/#) */
    uint8_t qos;                     /* 0, 1, or 2 */
    bool retained;                   /* Retain flag */
} msl_mqtt_descriptor_t;

typedef struct {
    uint16_t service_uuid;   /* BLE GATT service UUID (16-bit) */
    uint16_t char_uuid;      /* Characteristic UUID */
    bool notify;             /* true = notifications enabled */
    bool indicate;           /* true = indications enabled */
} msl_ble_descriptor_t;

typedef struct {
    uint8_t slave_addr;      /* Modbus slave address (1-247) */
    uint8_t function_code;   /* FC 01-06, 15, 16 */
    uint16_t reg_addr;       /* Starting register address */
    uint16_t reg_count;      /* Number of registers */
} msl_modbus_descriptor_t;

typedef struct {
    char url[MSL_MAX_TOPIC_LEN];  /* Endpoint URL */
    uint16_t port;                 /* Port number */
    uint8_t method;                /* 0=GET, 1=POST, 2=PUT, 3=DELETE */
    uint32_t interval_ms;          /* Polling interval (0 = event-driven) */
} msl_http_descriptor_t;

typedef struct {
    char url[MSL_MAX_TOPIC_LEN];  /* WebSocket URL */
    char msg_type[32];             /* Message type field for dispatch */
} msl_websocket_descriptor_t;

typedef struct {
    char uri[MSL_MAX_TOPIC_LEN];  /* CoAP resource URI */
    uint8_t method;                /* 0=GET, 1=POST, 2=PUT, 3=DELETE */
    bool observe;                  /* true = OBSERVE mode */
    bool confirmable;              /* CON vs NON */
} msl_coap_descriptor_t;

typedef struct {
    uint8_t dest_addr;       /* LoRa destination address (0xFF = broadcast) */
    uint8_t msg_type;        /* Application message type */
    int8_t sf;               /* Spreading factor (7-12) */
} msl_lora_descriptor_t;

typedef struct {
    uint8_t fport;           /* LoRaWAN FPort (1-223) */
    bool confirmed;          /* Confirmed vs unconfirmed */
} msl_lorawan_descriptor_t;

typedef struct {
    uint16_t cluster_id;     /* Zigbee cluster */
    uint16_t attr_id;        /* Attribute ID */
    uint8_t endpoint;        /* Endpoint number */
} msl_zigbee_descriptor_t;

typedef struct {
    uint8_t msg_id;          /* UART frame message type ID */
    uint8_t src_addr;        /* Source node address (for multi-drop) */
} msl_uart_descriptor_t;

typedef struct {
    uint8_t sender_mac[6];   /* ESP-NOW sender MAC (or 0xFF for any) */
    uint8_t msg_type;        /* First byte of payload = type */
} msl_espnow_descriptor_t;

typedef struct {
    uint32_t src_ip;         /* Source IP (0 = any) */
    uint16_t src_port;       /* Source port (0 = any) */
    uint8_t msg_type;        /* Application message type */
} msl_udp_descriptor_t;

typedef struct {
    uint8_t msg_type;        /* Frame type byte */
    uint32_t peer_ip;        /* Peer IP (0 = any connected client) */
} msl_tcp_descriptor_t;

typedef struct {
    uint64_t rom_code;       /* 1-Wire 64-bit ROM address (0 = any) */
    uint8_t family;          /* Device family code (e.g. 0x28 = DS18B20) */
} msl_onewire_descriptor_t;

typedef struct {
    uint8_t ndef_type;       /* NDEF record type (0=URI, 1=Text, 2=MIME) */
    uint8_t uid[7];          /* Tag UID filter (0 = any) */
    uint8_t uid_len;
} msl_nfc_descriptor_t;

typedef struct {
    uint16_t short_addr;     /* UWB short address (0 = any) */
    bool is_ranging;         /* true = range data, false = app data */
} msl_uwb_descriptor_t;

typedef struct {
    char node_id[32];        /* OPC-UA NodeId string */
    uint32_t ns_index;       /* Namespace index */
} msl_opcua_descriptor_t;

typedef struct {
    uint16_t slave_pos;      /* EtherCAT slave position in ring */
    uint16_t pdo_index;      /* PDO mapping index */
} msl_ethercat_descriptor_t;

typedef struct {
    uint16_t slot;           /* PROFINET module slot */
    uint16_t subslot;        /* Subslot within module */
    uint16_t offset;         /* Byte offset in IO frame */
    uint16_t length;         /* Data length in bytes */
} msl_profinet_descriptor_t;

/* Unified protocol descriptor (discriminated union) */
typedef struct {
    msl_protocol_t protocol;
    union {
        msl_can_descriptor_t can;
        msl_canopen_descriptor_t canopen;
        msl_mqtt_descriptor_t mqtt;
        msl_ble_descriptor_t ble;
        msl_modbus_descriptor_t modbus;
        msl_http_descriptor_t http;
        msl_websocket_descriptor_t websocket;
        msl_coap_descriptor_t coap;
        msl_lora_descriptor_t lora;
        msl_lorawan_descriptor_t lorawan;
        msl_zigbee_descriptor_t zigbee;
        msl_uart_descriptor_t uart;
        msl_espnow_descriptor_t espnow;
        msl_udp_descriptor_t udp;
        msl_tcp_descriptor_t tcp;
        msl_onewire_descriptor_t onewire;
        msl_nfc_descriptor_t nfc;
        msl_uwb_descriptor_t uwb;
        msl_opcua_descriptor_t opcua;
        msl_ethercat_descriptor_t ethercat;
        msl_profinet_descriptor_t profinet;
    };
} msl_descriptor_t;

/* Message payload container */
typedef struct {
    msl_protocol_t protocol;
    msl_plane_t plane;
    const uint8_t *data;
    size_t length;
    uint32_t timestamp_ms;
    union {
        struct { uint32_t can_id; bool extended; } can_meta;
        struct { const char *topic; uint8_t qos; } mqtt_meta;
        struct { uint16_t char_uuid; uint16_t conn_handle; } ble_meta;
        struct { uint8_t slave_addr; uint16_t reg_addr; } modbus_meta;
        struct { uint8_t sender_mac[6]; } espnow_meta;
        struct { uint32_t src_ip; uint16_t src_port; } udp_meta;
        struct { uint8_t fport; int16_t rssi; } lorawan_meta;
    } meta;
} msl_message_t;

/* Subscription callback signature */
typedef void (*msl_handler_t)(const msl_message_t *msg, void *user_ctx);

/* Error callback signature */
typedef void (*msl_error_handler_t)(msl_protocol_t protocol, int error_code, const char *msg);

/* Lifecycle state change callback */
typedef void (*msl_state_handler_t)(msl_protocol_t protocol, msl_state_t old_state, msl_state_t new_state);

/* Subscription handle (returned by msl_subscribe) */
typedef uint16_t msl_sub_handle_t;
#define MSL_INVALID_HANDLE  0xFFFF

#ifdef __cplusplus
}
#endif

#endif /* MSL_TYPES_H */