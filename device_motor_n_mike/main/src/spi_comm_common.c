/**
 * @file spi_comm_common.c
 * @brief SPI Common Utilities Implementation

 */

#include "spi_comm_common.h"
#include "osal/osal_log.h"
#include "pal/pal_spi.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include <string.h>

OSAL_LOG_MODULE_REGISTER(spi_common)

esp_err_t spi_bus_init_common(spi_host_device_t host, int mosi, int miso, int sclk)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    
    return spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);
}

esp_err_t spi_bus_deinit_common(spi_host_device_t host)
{
    return spi_bus_free(host);
}

esp_err_t spi_device_add(spi_host_device_t host, int cs_pin, uint32_t clock_hz,
                         uint8_t mode, spi_device_handle_t *handle)
{
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = clock_hz,
        .mode = mode,
        .spics_io_num = cs_pin,
        .queue_size = 7,
        .flags = 0,
    };
    
    return spi_bus_add_device(host, &dev_cfg, handle);
}

esp_err_t spi_device_remove(spi_device_handle_t handle)
{
    return spi_bus_remove_device(handle);
}

esp_err_t spi_transfer(spi_device_handle_t handle, const uint8_t *tx_data,
                       uint8_t *rx_data, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    
    return spi_device_transmit(handle, &t);
}

esp_err_t spi_write(spi_device_handle_t handle, const uint8_t *data, size_t len)
{
    return spi_transfer(handle, data, NULL, len);
}

esp_err_t spi_read(spi_device_handle_t handle, uint8_t *data, size_t len)
{
    return spi_transfer(handle, NULL, data, len);
}

esp_err_t spi_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2] = {reg | 0x80, value};  // Most SPI devices use MSB for write
    return spi_write(handle, tx_data, 2);
}

esp_err_t spi_read_reg(spi_device_handle_t handle, uint8_t reg, uint8_t *value)
{
    uint8_t tx_data[2] = {reg & 0x7F, 0x00};
    uint8_t rx_data[2];
    
    esp_err_t ret = spi_transfer(handle, tx_data, rx_data, 2);
    if (ret == ESP_OK && value) {
        *value = rx_data[1];
    }
    return ret;
}

esp_err_t spi_write_burst(spi_device_handle_t handle, uint8_t reg,
                          const uint8_t *data, size_t len)
{
    uint8_t *tx_buf = malloc(len + 1);
    if (!tx_buf) {
        return ESP_ERR_NO_MEM;
    }
    
    tx_buf[0] = reg | 0x80;
    memcpy(tx_buf + 1, data, len);
    
    esp_err_t ret = spi_write(handle, tx_buf, len + 1);
    free(tx_buf);
    return ret;
}

esp_err_t spi_read_burst(spi_device_handle_t handle, uint8_t reg,
                         uint8_t *data, size_t len)
{
    uint8_t *tx_buf = calloc(len + 1, 1);
    uint8_t *rx_buf = malloc(len + 1);
    if (!tx_buf || !rx_buf) {
        free(tx_buf);
        free(rx_buf);
        return ESP_ERR_NO_MEM;
    }
    
    tx_buf[0] = reg & 0x7F;
    
    esp_err_t ret = spi_transfer(handle, tx_buf, rx_buf, len + 1);
    if (ret == ESP_OK) {
        memcpy(data, rx_buf + 1, len);
    }
    
    free(tx_buf);
    free(rx_buf);
    return ret;
}