/**
 * @file spi_slave_comm.h
 * @brief SPI Slave Communication - Bidirectional Sensor MCU
 * 
 * This MCU acts as the SPI SLAVE (sensor) in bidirectional communication:
 * - Responds to master-initiated transactions
 * - SENDS: Sensor data to master during transactions
 * - RECEIVES: Configuration commands from master
 * 
 * SPI slave must have data ready when master initiates transaction.
 * 
 * Wiring (direct connections, no breadboard):
 * - Slave MOSI (GPIO23) <- Master MOSI
 * - Slave MISO (GPIO19) -> Master MISO
 * - Slave SCK  (GPIO18)  <- Master SCK
 * - Slave CS   (GPIO5)   <- Master CS
 * - GND -> GND (common ground required!)
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#ifndef SPI_SLAVE_COMM_H
#define SPI_SLAVE_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "osal/osal_types.h"
#include "spi_comm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration from template */
#define SPI_COMM_MOSI_PIN           23
#define SPI_COMM_MISO_PIN           19
#define SPI_COMM_SCK_PIN            18
#define SPI_COMM_CS_PIN             5
#define SPI_COMM_MCU_ID             2
#define SPI_COMM_DEFAULT_INTERVAL   1000

/* Number of sensors */
#define SPI_COMM_SENSOR_COUNT       1

/**
 * @brief Callback for received configuration commands
 */
typedef void (*spi_slave_config_callback_t)(const spi_config_command_t *cmd);

/**
 * @brief Initialize SPI slave communication
 */
esp_err_t spi_slave_init(void);

/**
 * @brief Deinitialize SPI slave communication
 */
esp_err_t spi_slave_deinit(void);

/**
 * @brief Set callback for configuration commands from master
 */
void spi_slave_set_config_callback(spi_slave_config_callback_t callback);

/**
 * @brief Update a sensor value (will be sent on next master request)
 */
void spi_slave_update_sensor(uint8_t sensor_id, spi_sensor_type_t sensor_type, float value);

/**
 * @brief Prepare all sensors for transmission
 * Call after updating all sensor values
 */
void spi_slave_prepare_response(void);

/* Convenience functions for common sensor types */
void spi_slave_set_inmp441_i2s_mems_microphone(float value);

/**
 * @brief Get current transmission interval (may be changed by master)
 */
uint32_t spi_slave_get_interval(void);

/**
 * @brief Get communication statistics
 */
void spi_slave_get_stats(uint32_t *transactions, uint32_t *errors);

#ifdef __cplusplus
}
#endif

#endif // SPI_SLAVE_COMM_H