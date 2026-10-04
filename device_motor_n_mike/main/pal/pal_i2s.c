/**
 * @file pal_i2s.c
 * @brief PAL I2S implementation for ESP-IDF
 */
#include "pal_i2s.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "pal_i2s";
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;

int pal_i2s_init(int port, const pal_i2s_config_t *config)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(port, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = config->dma_buf_count ? config->dma_buf_count : 6;
    chan_cfg.dma_frame_num = config->dma_buf_len ? config->dma_buf_len : 240;

    esp_err_t err;
    if (config->mode == PAL_I2S_MODE_MASTER_TX || config->mode == PAL_I2S_MODE_MASTER_DUPLEX) {
        err = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
        if (err != ESP_OK) { ESP_LOGE(TAG, "TX channel failed"); return -1; }
    }
    if (config->mode == PAL_I2S_MODE_MASTER_RX || config->mode == PAL_I2S_MODE_MASTER_DUPLEX) {
        err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
        if (err != ESP_OK) { ESP_LOGE(TAG, "RX channel failed"); return -1; }
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config->sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(config->bits_per_sample, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)config->bck_pin,
            .ws = (gpio_num_t)config->ws_pin,
            .dout = (gpio_num_t)config->data_out_pin,
            .din = (gpio_num_t)config->data_in_pin,
        },
    };

    if (tx_handle) { i2s_channel_init_std_mode(tx_handle, &std_cfg); i2s_channel_enable(tx_handle); }
    if (rx_handle) { i2s_channel_init_std_mode(rx_handle, &std_cfg); i2s_channel_enable(rx_handle); }

    ESP_LOGI(TAG, "I2S%d init: %luHz %d-bit", port, (unsigned long)config->sample_rate, config->bits_per_sample);
    return 0;
}

void pal_i2s_deinit(int port)
{
    (void)port;
    if (tx_handle) { i2s_channel_disable(tx_handle); i2s_del_channel(tx_handle); tx_handle = NULL; }
    if (rx_handle) { i2s_channel_disable(rx_handle); i2s_del_channel(rx_handle); rx_handle = NULL; }
}

int pal_i2s_write(int port, const void *data, size_t len, uint32_t timeout_ms)
{
    (void)port;
    size_t written = 0;
    esp_err_t err = i2s_channel_write(tx_handle, data, len, &written, timeout_ms);
    return (err == ESP_OK) ? (int)written : -1;
}

int pal_i2s_read(int port, void *data, size_t len, uint32_t timeout_ms)
{
    (void)port;
    size_t read_bytes = 0;
    esp_err_t err = i2s_channel_read(rx_handle, data, len, &read_bytes, timeout_ms);
    return (err == ESP_OK) ? (int)read_bytes : -1;
}

int pal_i2s_set_sample_rate(int port, uint32_t rate)
{
    (void)port;
    /* Requires reinit in ESP-IDF v5 */
    ESP_LOGW(TAG, "Runtime sample rate change requires reinit");
    return -1;
}