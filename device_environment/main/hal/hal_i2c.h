/**
 * @file hal_i2c.h
 * @brief Hardware Abstraction Layer - I2C Interface

 */

#ifndef HAL_I2C_H
#define HAL_I2C_H

#include "hal_types.h"
#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_I2C_SPEED_STANDARD = 100000,   /* 100 kHz */
    HAL_I2C_SPEED_FAST     = 400000,   /* 400 kHz */
    HAL_I2C_SPEED_FAST_PLUS = 1000000, /* 1 MHz   */
} hal_i2c_speed_t;

typedef struct {
    hal_i2c_port_t port;
    hal_gpio_pin_t sda_pin;
    hal_gpio_pin_t scl_pin;
    uint32_t clock_hz;
    bool pullup_enable;
} hal_i2c_config_t;

#define HAL_I2C_CONFIG_DEFAULT() { \
    .port = 0,                      \
    .sda_pin = HAL_GPIO_PIN_INVALID,\
    .scl_pin = HAL_GPIO_PIN_INVALID,\
    .clock_hz = HAL_I2C_DEFAULT_CLOCK_HZ, \
    .pullup_enable = true,          \
}

hal_err_t hal_i2c_init(const hal_i2c_config_t* config);

hal_err_t hal_i2c_deinit(hal_i2c_port_t port);

/**
 * @brief Write data to I2C device
 * @param port I2C port
 * @param addr 7-bit device address
 * @param data Data buffer to write
 * @param len Number of bytes
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success
 */
hal_err_t hal_i2c_write(hal_i2c_port_t port, uint8_t addr,
                         const uint8_t* data, size_t len, hal_ms_t timeout_ms);

/**
 * @brief Read data from I2C device
 */
hal_err_t hal_i2c_read(hal_i2c_port_t port, uint8_t addr,
                        uint8_t* data, size_t len, hal_ms_t timeout_ms);

/**
 * @brief Write then read (combined transaction)
 */
hal_err_t hal_i2c_write_read(hal_i2c_port_t port, uint8_t addr,
                              const uint8_t* write_data, size_t write_len,
                              uint8_t* read_data, size_t read_len,
                              hal_ms_t timeout_ms);

/**
 * @brief Write to a register address
 */
hal_err_t hal_i2c_write_reg(hal_i2c_port_t port, uint8_t addr,
                             uint8_t reg, const uint8_t* data, size_t len,
                             hal_ms_t timeout_ms);

/**
 * @brief Read from a register address
 */
hal_err_t hal_i2c_read_reg(hal_i2c_port_t port, uint8_t addr,
                            uint8_t reg, uint8_t* data, size_t len,
                            hal_ms_t timeout_ms);

/**
 * @brief Scan I2C bus for devices
 * @param port I2C port
 * @param found_addrs Array to store found addresses (at least 128 entries)
 * @param max_addrs Size of found_addrs array
 * @param count Pointer to store number of found devices
 * @return HAL_OK on success
 */
hal_err_t hal_i2c_scan(hal_i2c_port_t port, uint8_t* found_addrs,
                        size_t max_addrs, size_t* count);

#ifdef __cplusplus
}
#endif

#endif /* HAL_I2C_H */