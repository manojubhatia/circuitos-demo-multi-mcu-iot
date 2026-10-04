/**
 * @file hal_spi_esp.c
 * @brief Hardware Abstraction Layer - SPI ESP-IDF Implementation

 */

#include "hal_spi.h"

#if HAL_PLATFORM_ESP_IDF

#include "driver/spi_master.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "hal_spi";

typedef struct {
    bool initialized;
    spi_device_handle_t dev_handle;
    hal_spi_config_t config;
} spi_host_state_t;

static spi_host_state_t s_hosts[HAL_SPI_MAX_HOSTS];

static spi_host_device_t _map_host(hal_spi_host_t host) {
#ifdef SPI3_HOST
    return (host == 0) ? SPI2_HOST : SPI3_HOST;
#else
    (void)host;
    return SPI2_HOST;
#endif
}

hal_err_t hal_spi_init(const hal_spi_config_t* config) {
    if (!config || config->host < 0 || config->host >= HAL_SPI_MAX_HOSTS) {
        return HAL_ERR_INVALID_ARG;
    }

    spi_host_state_t* hs = &s_hosts[config->host];
    if (hs->initialized) return HAL_OK;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = config->mosi_pin,
        .miso_io_num = config->miso_pin,
        .sclk_io_num = config->sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t err = spi_bus_initialize(_map_host(config->host), &bus_cfg,
                                        config->dma_channel > 0 ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = (int)config->clock_hz,
        .mode = config->mode,
        .spics_io_num = config->cs_pin,
        .queue_size = 4,
        .flags = (config->bit_order == HAL_SPI_BIT_ORDER_LSB_FIRST) ? SPI_DEVICE_BIT_LSBFIRST : 0,
    };

    err = spi_bus_add_device(_map_host(config->host), &dev_cfg, &hs->dev_handle);
    if (err != ESP_OK) {
        spi_bus_free(_map_host(config->host));
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }

    hs->config = *config;
    hs->initialized = true;
    ESP_LOGI(TAG, "SPI host %d init: MOSI=%d, MISO=%d, SCLK=%d, CS=%d, %lu Hz",
             config->host, (int)config->mosi_pin, (int)config->miso_pin,
             (int)config->sclk_pin, (int)config->cs_pin, (unsigned long)config->clock_hz);
    return HAL_OK;
}

hal_err_t hal_spi_deinit(hal_spi_host_t host) {
    if (host < 0 || host >= HAL_SPI_MAX_HOSTS) return HAL_ERR_INVALID_ARG;
    spi_host_state_t* hs = &s_hosts[host];
    if (!hs->initialized) return HAL_OK;
    spi_bus_remove_device(hs->dev_handle);
    spi_bus_free(_map_host(host));
    memset(hs, 0, sizeof(*hs));
    return HAL_OK;
}

hal_err_t hal_spi_transfer(hal_spi_host_t host,
                            const uint8_t* tx_data, uint8_t* rx_data,
                            size_t len) {
    if (host < 0 || host >= HAL_SPI_MAX_HOSTS || !s_hosts[host].initialized) return HAL_ERR_NOT_INITIALIZED;
    if (len == 0) return HAL_OK;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = tx_data;
    t.rx_buffer = rx_data;
    if (!rx_data) t.rxlength = 0;

    esp_err_t err = spi_device_polling_transmit(s_hosts[host].dev_handle, &t);
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_spi_write(hal_spi_host_t host, const uint8_t* data, size_t len) {
    return hal_spi_transfer(host, data, NULL, len);
}

hal_err_t hal_spi_read(hal_spi_host_t host, uint8_t* data, size_t len) {
    return hal_spi_transfer(host, NULL, data, len);
}

hal_err_t hal_spi_cs_select(hal_spi_host_t host) {
    (void)host;
    return HAL_OK;
}

hal_err_t hal_spi_cs_deselect(hal_spi_host_t host) {
    (void)host;
    return HAL_OK;
}

#endif /* HAL_PLATFORM_ESP_IDF */