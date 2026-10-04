/**
 * @file pal_i2c.h
 * @brief Protocol Abstraction Layer - I2C Bus Interface
 */
#ifndef PAL_I2C_H
#define PAL_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAL_I2C_SPEED_100K  = 100000,
    PAL_I2C_SPEED_400K  = 400000,
    PAL_I2C_SPEED_1M    = 1000000,
} pal_i2c_speed_t;

typedef struct {
    int sda_pin;
    int scl_pin;
    pal_i2c_speed_t speed;
    bool pullup_enable;
} pal_i2c_config_t;

int pal_i2c_init(int port, const pal_i2c_config_t *config);
void pal_i2c_deinit(int port);
int pal_i2c_write(int port, uint8_t addr, const uint8_t *data, size_t len, uint32_t timeout_ms);
int pal_i2c_read(int port, uint8_t addr, uint8_t *data, size_t len, uint32_t timeout_ms);
int pal_i2c_write_reg(int port, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
int pal_i2c_read_reg(int port, uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
int pal_i2c_probe(int port, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* PAL_I2C_H */