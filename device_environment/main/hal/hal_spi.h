/**
 * @file hal_spi.h
 * @brief Hardware Abstraction Layer - SPI Interface

 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "hal_types.h"
#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_SPI_MODE_0 = 0,  /* CPOL=0, CPHA=0 */
    HAL_SPI_MODE_1 = 1,  /* CPOL=0, CPHA=1 */
    HAL_SPI_MODE_2 = 2,  /* CPOL=1, CPHA=0 */
    HAL_SPI_MODE_3 = 3,  /* CPOL=1, CPHA=1 */
} hal_spi_mode_t;

typedef enum {
    HAL_SPI_BIT_ORDER_MSB_FIRST,
    HAL_SPI_BIT_ORDER_LSB_FIRST,
} hal_spi_bit_order_t;

typedef struct {
    hal_spi_host_t host;
    hal_gpio_pin_t mosi_pin;
    hal_gpio_pin_t miso_pin;
    hal_gpio_pin_t sclk_pin;
    hal_gpio_pin_t cs_pin;       /* -1 for software CS */
    uint32_t clock_hz;
    hal_spi_mode_t mode;
    hal_spi_bit_order_t bit_order;
    int dma_channel;             /* 0=no DMA, 1/2=DMA channel */
} hal_spi_config_t;

#define HAL_SPI_CONFIG_DEFAULT() { \
    .host = 0,                      \
    .mosi_pin = HAL_GPIO_PIN_INVALID, \
    .miso_pin = HAL_GPIO_PIN_INVALID, \
    .sclk_pin = HAL_GPIO_PIN_INVALID, \
    .cs_pin = HAL_GPIO_PIN_INVALID,   \
    .clock_hz = HAL_SPI_DEFAULT_CLOCK_HZ, \
    .mode = HAL_SPI_MODE_0,         \
    .bit_order = HAL_SPI_BIT_ORDER_MSB_FIRST, \
    .dma_channel = 0,               \
}

hal_err_t hal_spi_init(const hal_spi_config_t* config);
hal_err_t hal_spi_deinit(hal_spi_host_t host);

hal_err_t hal_spi_transfer(hal_spi_host_t host,
                            const uint8_t* tx_data, uint8_t* rx_data,
                            size_t len);

hal_err_t hal_spi_write(hal_spi_host_t host,
                         const uint8_t* data, size_t len);

hal_err_t hal_spi_read(hal_spi_host_t host,
                        uint8_t* data, size_t len);

hal_err_t hal_spi_cs_select(hal_spi_host_t host);
hal_err_t hal_spi_cs_deselect(hal_spi_host_t host);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SPI_H */