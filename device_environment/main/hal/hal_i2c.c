/**
 * @file hal_i2c_esp.c
 * @brief Hardware Abstraction Layer - I2C ESP-IDF Implementation

 */

#include "hal_i2c.h"

#if HAL_PLATFORM_ESP_IDF

#include "driver/i2c_master.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "hal_i2c";

typedef struct {
    bool initialized;
    i2c_master_bus_handle_t bus_handle;
    hal_i2c_config_t config;
} i2c_port_state_t;

static i2c_port_state_t s_ports[HAL_I2C_MAX_PORTS];

hal_err_t hal_i2c_init(const hal_i2c_config_t* config) {
    if (!config || config->port < 0 || config->port >= HAL_I2C_MAX_PORTS) {
        return HAL_ERR_INVALID_ARG;
    }

    i2c_port_state_t* ps = &s_ports[config->port];
    if (ps->initialized) return HAL_OK;

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = config->port,
        .sda_io_num = config->sda_pin,
        .scl_io_num = config->scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = config->pullup_enable,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &ps->bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus on port %d: %s",
                 config->port, esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }

    ps->config = *config;
    ps->initialized = true;
    ESP_LOGI(TAG, "I2C port %d init: SDA=%d, SCL=%d, %lu Hz",
             config->port, (int)config->sda_pin, (int)config->scl_pin,
             (unsigned long)config->clock_hz);
    return HAL_OK;
}

hal_err_t hal_i2c_deinit(hal_i2c_port_t port) {
    if (port < 0 || port >= HAL_I2C_MAX_PORTS) return HAL_ERR_INVALID_ARG;
    i2c_port_state_t* ps = &s_ports[port];
    if (!ps->initialized) return HAL_OK;

    i2c_del_master_bus(ps->bus_handle);
    memset(ps, 0, sizeof(*ps));
    return HAL_OK;
}

static i2c_master_dev_handle_t _get_dev(hal_i2c_port_t port, uint8_t addr, uint32_t speed) {
    i2c_port_state_t* ps = &s_ports[port];
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = speed ? speed : ps->config.clock_hz,
    };
    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = i2c_master_bus_add_device(ps->bus_handle, &dev_cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device 0x%02X: %s", addr, esp_err_to_name(err));
    }
    return dev;
}

hal_err_t hal_i2c_write(hal_i2c_port_t port, uint8_t addr,
                         const uint8_t* data, size_t len, hal_ms_t timeout_ms) {
    if (port < 0 || port >= HAL_I2C_MAX_PORTS || !s_ports[port].initialized) return HAL_ERR_NOT_INITIALIZED;
    if (!data || len == 0) return HAL_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev = _get_dev(port, addr, 0);
    if (!dev) return HAL_ERR_FAILED;

    esp_err_t err = i2c_master_transmit(dev, data, len, (int)timeout_ms);
    i2c_master_bus_rm_device(dev);
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_i2c_read(hal_i2c_port_t port, uint8_t addr,
                        uint8_t* data, size_t len, hal_ms_t timeout_ms) {
    if (port < 0 || port >= HAL_I2C_MAX_PORTS || !s_ports[port].initialized) return HAL_ERR_NOT_INITIALIZED;
    if (!data || len == 0) return HAL_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev = _get_dev(port, addr, 0);
    if (!dev) return HAL_ERR_FAILED;

    esp_err_t err = i2c_master_receive(dev, data, len, (int)timeout_ms);
    i2c_master_bus_rm_device(dev);
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_i2c_write_read(hal_i2c_port_t port, uint8_t addr,
                              const uint8_t* write_data, size_t write_len,
                              uint8_t* read_data, size_t read_len,
                              hal_ms_t timeout_ms) {
    if (port < 0 || port >= HAL_I2C_MAX_PORTS || !s_ports[port].initialized) return HAL_ERR_NOT_INITIALIZED;

    i2c_master_dev_handle_t dev = _get_dev(port, addr, 0);
    if (!dev) return HAL_ERR_FAILED;

    esp_err_t err = i2c_master_transmit_receive(dev, write_data, write_len,
                                                  read_data, read_len, (int)timeout_ms);
    i2c_master_bus_rm_device(dev);
    return (err == ESP_OK) ? HAL_OK : HAL_ERR_FAILED;
}

hal_err_t hal_i2c_write_reg(hal_i2c_port_t port, uint8_t addr,
                             uint8_t reg, const uint8_t* data, size_t len,
                             hal_ms_t timeout_ms) {
    uint8_t buf[64];
    if (len + 1 > sizeof(buf)) return HAL_ERR_NO_MEMORY;
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return hal_i2c_write(port, addr, buf, len + 1, timeout_ms);
}

hal_err_t hal_i2c_read_reg(hal_i2c_port_t port, uint8_t addr,
                            uint8_t reg, uint8_t* data, size_t len,
                            hal_ms_t timeout_ms) {
    return hal_i2c_write_read(port, addr, &reg, 1, data, len, timeout_ms);
}

hal_err_t hal_i2c_scan(hal_i2c_port_t port, uint8_t* found_addrs,
                        size_t max_addrs, size_t* count) {
    if (port < 0 || port >= HAL_I2C_MAX_PORTS || !s_ports[port].initialized) return HAL_ERR_NOT_INITIALIZED;
    if (!found_addrs || !count) return HAL_ERR_INVALID_ARG;

    *count = 0;
    for (uint8_t addr = 0x08; addr < 0x78 && *count < max_addrs; addr++) {
        i2c_master_dev_handle_t dev = _get_dev(port, addr, 100000);
        if (!dev) continue;
        esp_err_t err = i2c_master_probe(s_ports[port].bus_handle, addr, 50);
        i2c_master_bus_rm_device(dev);
        if (err == ESP_OK) {
            found_addrs[(*count)++] = addr;
        }
    }
    ESP_LOGI(TAG, "I2C scan: found %d devices on port %d", (int)*count, port);
    return HAL_OK;
}

#endif /* HAL_PLATFORM_ESP_IDF */