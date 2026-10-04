/**
 * @file pal_spi.h
 * @brief Protocol Abstraction Layer - SPI Bus Interface
 */
#ifndef PAL_SPI_H
#define PAL_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAL_SPI_MODE_0 = 0,  /**< CPOL=0, CPHA=0 */
    PAL_SPI_MODE_1,      /**< CPOL=0, CPHA=1 */
    PAL_SPI_MODE_2,      /**< CPOL=1, CPHA=0 */
    PAL_SPI_MODE_3,      /**< CPOL=1, CPHA=1 */
} pal_spi_mode_t;

typedef struct {
    int mosi_pin;
    int miso_pin;
    int sclk_pin;
    int cs_pin;
    uint32_t clock_hz;
    pal_spi_mode_t mode;
    uint8_t bit_order;  /**< 0 = MSB first, 1 = LSB first */
} pal_spi_config_t;

typedef void *pal_spi_handle_t;

int pal_spi_init(int host, const pal_spi_config_t *config, pal_spi_handle_t *handle);
void pal_spi_deinit(pal_spi_handle_t handle);
int pal_spi_transfer(pal_spi_handle_t handle, const uint8_t *tx, uint8_t *rx, size_t len);
int pal_spi_write(pal_spi_handle_t handle, const uint8_t *data, size_t len);
int pal_spi_read(pal_spi_handle_t handle, uint8_t *data, size_t len);
int pal_spi_write_reg(pal_spi_handle_t handle, uint8_t reg, uint8_t value);
uint8_t pal_spi_read_reg(pal_spi_handle_t handle, uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif /* PAL_SPI_H */