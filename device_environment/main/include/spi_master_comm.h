/**
 * @file spi_master_comm.h
 * @brief SPI Master Communication - Bidirectional Controller MCU
 * 
 * This MCU acts as the SPI MASTER (controller) in bidirectional communication:
 * - Controls clock (SCK) and chip select (CS)
 * - Initiates all transactions
 * - RECEIVES: Sensor data from slave during transactions
 * - SENDS: Configuration commands to slave
 * 
 * SPI is full-duplex: master sends command while receiving response simultaneously.
 * 
 * Wiring (direct connections, no breadboard):
 * - Master MOSI (GPIO23) -> Slave MOSI
 * - Master MISO (GPIO19) -> Slave MISO
 * - Master SCK  (GPIO18)  -> Slave SCK
 * - Master CS   (GPIO5)   -> Slave CS
 * - GND -> GND (common ground required!)
 * 
 * Auto-generated - DO NOT EDIT MANUALLY
 */

#ifndef SPI_MASTER_COMM_H
#define SPI_MASTER_COMM_H

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
#define SPI_COMM_CLOCK_SPEED_HZ     1000000
#define SPI_COMM_MCU_ID             1

/* Number of expected sensors from slave */
#define SPI_COMM_EXPECTED_SENSORS   7

/**
 * @brief Callback for received sensor data
 */
typedef void (*spi_master_data_callback_t)(const spi_sensor_packet_t *packet);

/**
 * @brief Initialize SPI master communication
 * @return ESP_OK on success
 */
esp_err_t spi_master_init(void);

/**
 * @brief Deinitialize SPI master communication
 * @return ESP_OK on success
 */
esp_err_t spi_master_deinit(void);

/**
 * @brief Set callback for received sensor data
 */
void spi_master_set_data_callback(spi_master_data_callback_t callback);

/* ═══════════════════════════════════════════════════════════════════
 * CONFIGURATION COMMANDS (Master -> Slave)
 * ═══════════════════════════════════════════════════════════════════ */

/**
 * @brief Set sensor transmission interval on slave
 */
esp_err_t spi_master_set_interval(uint32_t interval_ms);

/**
 * @brief Enable or disable a specific sensor on slave
 */
esp_err_t spi_master_enable_sensor(uint8_t sensor_id, bool enable);

/**
 * @brief Request sensor data from slave (full-duplex transaction)
 * @return ESP_OK on success, data available via callback or getter
 */
esp_err_t spi_master_request_data(void);

/**
 * @brief Send reset command to slave
 */
esp_err_t spi_master_reset_slave(void);

/**
 * @brief Perform a full-duplex transaction
 * @param tx_data Data to send to slave
 * @param rx_data Buffer for data from slave
 * @param len Length of transaction
 */
esp_err_t spi_master_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t len);

/* ═══════════════════════════════════════════════════════════════════
 * RECEIVED DATA ACCESS
 * ═══════════════════════════════════════════════════════════════════ */

float spi_master_get_joystick_up(void);
float spi_master_get_joystick_down(void);
float spi_master_get_joystick_left(void);
float spi_master_get_joystick_right(void);
float spi_master_get_joystick_center_press(void);
float spi_master_get_boot_button(void);
float spi_master_get_aht10_temperature_humidity_sensor(void);

float spi_master_get_sensor_value(uint8_t sensor_id);
bool spi_master_is_data_fresh(uint32_t timeout_ms);
uint32_t spi_master_get_data_age(void);
void spi_master_get_stats(uint32_t *transactions, uint32_t *errors);

#ifdef __cplusplus
}
#endif

#endif // SPI_MASTER_COMM_H