/**
 * @file BuiltinDisplay.cpp
 * @brief ILI9341 TFT Display Implementation
 */
#include "BuiltinDisplay.hpp"
#include "osal/osal_log.h"
#include "pal/pal_display.h"
#include "display_ui.h"
#if defined(ESP_PLATFORM) || defined(IDF_VER)
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "esp_heap_caps.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch_xpt2046.h"
#endif

#define LCD_H_RES 320
#define LCD_V_RES 240
#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIXEL_CLK_HZ (26 * 1000 * 1000)
OSAL_LOG_MODULE_REGISTER(BuiltinDisplay)
namespace embedded {
BuiltinDisplay::BuiltinDisplay(int csPin, int dcPin, int rstPin, int mosiPin, int sclkPin, int blPin)
    : Display("BuiltinDisplay", LCD_H_RES, LCD_V_RES),
      _csPin(csPin), _dcPin(dcPin), _rstPin(rstPin),
      _mosiPin(mosiPin), _sclkPin(sclkPin), _blPin(blPin) {}

esp_err_t BuiltinDisplay::init() {
    if (_initialized) return ESP_OK;
    OSAL_LOGI("Initializing ILI9341 display (SPI: MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
             _mosiPin, _sclkPin, _csPin, _dcPin, _blPin);

    if (_blPin >= 0) {
        gpio_config_t bl_cfg = {};
        bl_cfg.pin_bit_mask = (1ULL << _blPin);
        bl_cfg.mode = GPIO_MODE_OUTPUT;
        gpio_config(&bl_cfg);
        gpio_set_level((gpio_num_t)_blPin, 1);
    }

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = (gpio_num_t)_mosiPin;
    bus_cfg.miso_io_num = GPIO_NUM_NC;
    bus_cfg.sclk_io_num = (gpio_num_t)_sclkPin;
    bus_cfg.quadwp_io_num = GPIO_NUM_NC;
    bus_cfg.quadhd_io_num = GPIO_NUM_NC;
    bus_cfg.max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num = (gpio_num_t)_dcPin;
    io_cfg.cs_gpio_num = (gpio_num_t)_csPin;
    io_cfg.pclk_hz = LCD_PIXEL_CLK_HZ;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    io_cfg.spi_mode = 0;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_cfg, &m_panelIo));

    ili9341_vendor_config_t vendor_cfg = {};
    static const ili9341_lcd_init_cmd_t ili9341_init_cmds[] = {
        {0xC5, (uint8_t[]){0x3E, 0x28}, 2, 0},
        {0xC7, (uint8_t[]){0x86}, 1, 0},
    };
    vendor_cfg.init_cmds = ili9341_init_cmds;
    vendor_cfg.init_cmds_size = sizeof(ili9341_init_cmds) / sizeof(ili9341_init_cmds[0]);

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = (gpio_num_t)_rstPin;
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel = 16;
    panel_cfg.vendor_config = &vendor_cfg;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(m_panelIo, &panel_cfg, &m_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(m_panel, true));

    // Portrait: rotate panel hardware via MADCTL so LVGL sees 240x320
    esp_lcd_panel_swap_xy(m_panel, true);
    esp_lcd_panel_mirror(m_panel, false, true);

    // Clear panel RAM to black before LVGL takes over
    {
        const int pw = 240, ph = 320;
        const int chunk_h = 20;
        uint16_t *black = (uint16_t *)heap_caps_calloc(pw * chunk_h, sizeof(uint16_t), MALLOC_CAP_DMA);
        if (black) {
            for (int y = 0; y < ph; y += chunk_h) {
                int h = ((y + chunk_h) <= ph) ? chunk_h : (ph - y);
                esp_lcd_panel_draw_bitmap(m_panel, 0, y, pw, y + h, black);
            }
            free(black);
        }
    }

    OSAL_LOGI("ILI9341 panel initialized");

    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_priority = 4;
    lvgl_cfg.task_stack = 8192;
    lvgl_cfg.timer_period_ms = 5;
    lvgl_cfg.task_max_sleep_ms = 100;
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle = m_panelIo;
    disp_cfg.panel_handle = m_panel;
    disp_cfg.buffer_size = 240 * 20 * sizeof(lv_color_t);
    disp_cfg.double_buffer = true;
    disp_cfg.hres = 240;
    disp_cfg.vres = 320;
    disp_cfg.monochrome = false;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;
    disp_cfg.rotation.swap_xy = false;
    disp_cfg.rotation.mirror_x = false;
    disp_cfg.rotation.mirror_y = false;
    disp_cfg.flags.swap_bytes = 1;
    disp_cfg.flags.buff_dma = 1;
    lvgl_port_add_disp(&disp_cfg);

    // Initialize XPT2046 touch controller on separate SPI bus
    {
        spi_bus_config_t touch_bus_cfg = {};
        touch_bus_cfg.mosi_io_num = 32;
        touch_bus_cfg.miso_io_num = 39;
        touch_bus_cfg.sclk_io_num = 25;
        touch_bus_cfg.quadwp_io_num = -1;
        touch_bus_cfg.quadhd_io_num = -1;
        touch_bus_cfg.max_transfer_sz = 0;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &touch_bus_cfg, SPI_DMA_CH_AUTO));

        esp_lcd_panel_io_handle_t touch_io = NULL;
        esp_lcd_panel_io_spi_config_t touch_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(33);
        touch_io_cfg.spi_mode = 0;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &touch_io_cfg, &touch_io));

        esp_lcd_touch_config_t touch_cfg = {};
        touch_cfg.x_max = 240;
        touch_cfg.y_max = 320;
        touch_cfg.rst_gpio_num = GPIO_NUM_NC;
        touch_cfg.int_gpio_num = GPIO_NUM_NC;  // Poll mode — more reliable than IRQ for XPT2046
        touch_cfg.flags.swap_xy = 1;
        touch_cfg.flags.mirror_x = 1;

        esp_lcd_touch_handle_t touch_handle = NULL;
        ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(touch_io, &touch_cfg, &touch_handle));

        lvgl_port_touch_cfg_t lvgl_touch_cfg = {};
        lvgl_touch_cfg.disp = lv_display_get_default();
        lvgl_touch_cfg.handle = touch_handle;
        lvgl_port_add_touch(&lvgl_touch_cfg);
        OSAL_LOGI("XPT2046 touch registered with LVGL");
    }

    OSAL_LOGI("LVGL port initialized, calling display_ui_init()...");
    if (lvgl_port_lock(5000)) {
        display_ui_init();
        lv_obj_invalidate(lv_scr_act());
        lv_refr_now(NULL);
        lvgl_port_unlock();
    }

    _initialized = true;
    OSAL_LOGI("Display fully initialized");
    return ESP_OK;
}

esp_err_t BuiltinDisplay::deinit() {
    _initialized = false;
    return ESP_OK;
}

void BuiltinDisplay::update() {
    // LVGL timer handler runs in the lvgl_port task automatically
}

void BuiltinDisplay::clear(uint16_t color) {
    // Not needed with LVGL
}
} // namespace embedded