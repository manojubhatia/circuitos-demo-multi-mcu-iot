/**
 * @file pal_i2c.c
 * @brief PAL I2C implementation for ESP-IDF v6.x (new i2c_master API)
 */
#include "pal_i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "pal_i2c";

#define PAL_I2C_MAX_PORTS 2
static i2c_master_bus_handle_t s_bus[PAL_I2C_MAX_PORTS] = {0};

int pal_i2c_init(int port, const pal_i2c_config_t *config)
{
    if (port < 0 || port >= PAL_I2C_MAX_PORTS) return -1;
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = port,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = config->pullup_enable,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus[port]);
    if (err != ESP_OK) { ESP_LOGE(TAG, "Bus init failed: %s", esp_err_to_name(err)); return -1; }
    ESP_LOGI(TAG, "I2C%d init SDA=%d SCL=%d", port, config->sda_pin, config->scl_pin);
    return 0;
}

void pal_i2c_deinit(int port)
{
    if (port >= 0 && port < PAL_I2C_MAX_PORTS && s_bus[port]) {
        i2c_del_master_bus(s_bus[port]);
        s_bus[port] = NULL;
    }
}

int pal_i2c_write(int port, uint8_t addr, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (port < 0 || port >= PAL_I2C_MAX_PORTS || !s_bus[port]) return -1;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t err = i2c_master_bus_add_device(s_bus[port], &dev_cfg, &dev);
    if (err != ESP_OK) return -1;
    err = i2c_master_transmit(dev, data, len, timeout_ms);
    i2c_master_bus_rm_device(dev);
    return (err == ESP_OK) ? 0 : -1;
}

int pal_i2c_read(int port, uint8_t addr, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    if (port < 0 || port >= PAL_I2C_MAX_PORTS || !s_bus[port]) return -1;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t err = i2c_master_bus_add_device(s_bus[port], &dev_cfg, &dev);
    if (err != ESP_OK) return -1;
    err = i2c_master_receive(dev, data, len, timeout_ms);
    i2c_master_bus_rm_device(dev);
    return (err == ESP_OK) ? 0 : -1;
}

int pal_i2c_write_reg(int port, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t buf[len + 1];
    buf[0] = reg;
    for (size_t i = 0; i < len; i++) buf[i + 1] = data[i];
    return pal_i2c_write(port, addr, buf, len + 1, 100);
}

int pal_i2c_read_reg(int port, uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (port < 0 || port >= PAL_I2C_MAX_PORTS || !s_bus[port]) return -1;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t err = i2c_master_bus_add_device(s_bus[port], &dev_cfg, &dev);
    if (err != ESP_OK) return -1;
    err = i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
    i2c_master_bus_rm_device(dev);
    return (err == ESP_OK) ? 0 : -1;
}

int pal_i2c_probe(int port, uint8_t addr)
{
    if (port < 0 || port >= PAL_I2C_MAX_PORTS || !s_bus[port]) return -1;
    esp_err_t err = i2c_master_probe(s_bus[port], addr, 50);
    return (err == ESP_OK) ? 0 : -1;
}