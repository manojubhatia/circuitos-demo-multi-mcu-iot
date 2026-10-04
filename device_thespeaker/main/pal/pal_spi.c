/**
 * @file pal_spi.c
 * @brief PAL SPI implementation for ESP-IDF
 */
#include "pal_spi.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "pal_spi";

typedef struct { spi_device_handle_t dev; } pal_spi_ctx_t;

int pal_spi_init(int host, const pal_spi_config_t *config, pal_spi_handle_t *handle)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = config->mosi_pin,
        .miso_io_num = config->miso_pin,
        .sclk_io_num = config->sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    spi_host_device_t spi_host = SPI2_HOST;
    (void)host;
    esp_err_t err = spi_bus_initialize(spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Bus init failed: %s", esp_err_to_name(err));
        return -1;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = (int)config->clock_hz,
        .mode = (uint8_t)config->mode,
        .spics_io_num = config->cs_pin,
        .queue_size = 7,
    };
    pal_spi_ctx_t *ctx = calloc(1, sizeof(pal_spi_ctx_t));
    err = spi_bus_add_device(spi_host, &dev_cfg, &ctx->dev);
    if (err != ESP_OK) { free(ctx); ESP_LOGE(TAG, "Add device failed"); return -1; }
    *handle = ctx;
    ESP_LOGI(TAG, "SPI host%d init @ %lu Hz", host, (unsigned long)config->clock_hz);
    return 0;
}

void pal_spi_deinit(pal_spi_handle_t handle)
{
    pal_spi_ctx_t *ctx = (pal_spi_ctx_t *)handle;
    if (ctx) { spi_bus_remove_device(ctx->dev); free(ctx); }
}

int pal_spi_transfer(pal_spi_handle_t handle, const uint8_t *tx, uint8_t *rx, size_t len)
{
    pal_spi_ctx_t *ctx = (pal_spi_ctx_t *)handle;
    spi_transaction_t t = { .length = len * 8, .tx_buffer = tx, .rx_buffer = rx };
    return (spi_device_polling_transmit(ctx->dev, &t) == ESP_OK) ? 0 : -1;
}

int pal_spi_write(pal_spi_handle_t handle, const uint8_t *data, size_t len)
{
    return pal_spi_transfer(handle, data, NULL, len);
}

int pal_spi_read(pal_spi_handle_t handle, uint8_t *data, size_t len)
{
    return pal_spi_transfer(handle, NULL, data, len);
}

int pal_spi_write_reg(pal_spi_handle_t handle, uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    return pal_spi_write(handle, tx, 2);
}

uint8_t pal_spi_read_reg(pal_spi_handle_t handle, uint8_t reg)
{
    uint8_t tx[2] = {reg | 0x80, 0};
    uint8_t rx[2] = {0};
    pal_spi_transfer(handle, tx, rx, 2);
    return rx[1];
}