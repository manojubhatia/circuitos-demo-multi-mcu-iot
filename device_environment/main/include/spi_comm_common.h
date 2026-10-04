/**
 * @file spi_comm_common.h
 * @brief Common SPI Communication Definitions
 * 
 * Shared types and constants for bidirectional SPI communication.
 * Master and slave MCUs include this header.
 * 
 * SPI is full-duplex: data flows both directions simultaneously.
 * Master controls clock (SCK) and chip select (CS).
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#ifndef SPI_COMM_COMMON_H
#define SPI_COMM_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol Constants */
#define SPI_COMM_START_MARKER       0x55
#define SPI_COMM_MAX_PACKET_SIZE    256
#define SPI_COMM_MAX_SENSORS        16

/* Frame Types for Bidirectional Communication */
typedef enum {
    SPI_FRAME_TYPE_SENSOR_DATA  = 0x01,  // Sensor readings (slave -> master)
    SPI_FRAME_TYPE_CONFIG       = 0x02,  // Configuration command (master -> slave)
    SPI_FRAME_TYPE_ACK          = 0x03,  // Acknowledgment
    SPI_FRAME_TYPE_NACK         = 0x04,  // Negative acknowledgment
    SPI_FRAME_TYPE_STATUS       = 0x05,  // Status query/response
    SPI_FRAME_TYPE_REQUEST      = 0x06,  // Request data (master -> slave)
    SPI_FRAME_TYPE_RESPONSE     = 0x07,  // Response to request
    SPI_FRAME_TYPE_NOP          = 0xFF,  // No operation (padding)
} spi_frame_type_t;

/* Configuration Command Types */
typedef enum {
    SPI_CONFIG_CMD_SET_INTERVAL     = 0x01,
    SPI_CONFIG_CMD_ENABLE_SENSOR    = 0x02,
    SPI_CONFIG_CMD_SET_THRESHOLD    = 0x03,
    SPI_CONFIG_CMD_RESET            = 0x04,
    SPI_CONFIG_CMD_GET_STATUS       = 0x05,
    SPI_CONFIG_CMD_SET_SPI_MODE     = 0x06,
} spi_config_cmd_t;

/* Sensor Type Identifiers (same as UART for consistency) */
typedef enum {
    SPI_SENSOR_TYPE_UNKNOWN     = 0x00,
    SPI_SENSOR_TYPE_TEMPERATURE = 0x01,
    SPI_SENSOR_TYPE_HUMIDITY    = 0x02,
    SPI_SENSOR_TYPE_PRESSURE    = 0x03,
    SPI_SENSOR_TYPE_LIGHT       = 0x04,
    SPI_SENSOR_TYPE_DISTANCE    = 0x05,
    SPI_SENSOR_TYPE_MOTION      = 0x06,
    SPI_SENSOR_TYPE_ACCELERATION= 0x07,
    SPI_SENSOR_TYPE_GYROSCOPE   = 0x08,
    SPI_SENSOR_TYPE_VOLTAGE     = 0x09,
    SPI_SENSOR_TYPE_CURRENT     = 0x0A,
    SPI_SENSOR_TYPE_GENERIC     = 0xFF
} spi_sensor_type_t;

/* Project-specific sensor IDs */
#define SPI_SENSOR_ID_JOYSTICK_UP    1
#define SPI_SENSOR_ID_JOYSTICK_DOWN    2
#define SPI_SENSOR_ID_JOYSTICK_LEFT    3
#define SPI_SENSOR_ID_JOYSTICK_RIGHT    4
#define SPI_SENSOR_ID_JOYSTICK_CENTER_PRESS    5
#define SPI_SENSOR_ID_BOOT_BUTTON    6
#define SPI_SENSOR_ID_AHT10_TEMPERATURE_HUMIDITY_SENSOR    7

/**
 * @brief Single sensor reading
 */
typedef struct __attribute__((packed)) {
    uint8_t sensor_id;
    uint8_t sensor_type;
    float value;
    uint32_t timestamp_ms;
} spi_sensor_reading_t;

/**
 * @brief Sensor data packet
 */
typedef struct __attribute__((packed)) {
    uint8_t mcu_id;
    uint8_t sensor_count;
    spi_sensor_reading_t sensors[SPI_COMM_MAX_SENSORS];
    uint32_t sequence_num;
} spi_sensor_packet_t;

/**
 * @brief Configuration command
 */
typedef struct __attribute__((packed)) {
    uint8_t cmd_type;
    uint8_t sensor_id;
    uint32_t value;
    uint32_t sequence_num;
} spi_config_command_t;

/**
 * @brief SPI transaction frame
 * Used for full-duplex communication
 */
typedef struct __attribute__((packed)) {
    uint8_t start_marker;
    uint8_t frame_type;
    uint16_t payload_len;
    uint8_t payload[SPI_COMM_MAX_PACKET_SIZE];
    uint16_t crc;
} spi_frame_t;

#ifdef __cplusplus
}
#endif

#endif // SPI_COMM_COMMON_H