/**
 * @file BuiltinDisplay.hpp
 * @brief ILI9341 320x240 TFT Display (SPI)
 */
#ifndef BUILTINDISPLAY_HPP
#define BUILTINDISPLAY_HPP
#include "base/Display.hpp"
#include "pal/pal_spi.h"
#include "pal/pal_gpio.h"
#include "pal/pal_display.h"
#if defined(ESP_PLATFORM) || defined(IDF_VER)
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#endif
namespace embedded {
class BuiltinDisplay : public Display {
public:
    BuiltinDisplay(int csPin, int dcPin, int rstPin = -1, int mosiPin = 23, int sclkPin = 18, int blPin = -1);
    esp_err_t init() override;
    esp_err_t deinit() override;
    void update() override;
    void clear(uint16_t color = 0x0000) override;
    void drawPixel(int16_t x, int16_t y, uint16_t color) override { (void)x; (void)y; (void)color; }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { (void)x; (void)y; (void)w; (void)h; (void)color; }
private:
    int _csPin, _dcPin, _rstPin, _mosiPin, _sclkPin, _blPin;
#if defined(ESP_PLATFORM) || defined(IDF_VER)
    esp_lcd_panel_io_handle_t m_panelIo = nullptr;
    esp_lcd_panel_handle_t m_panel = nullptr;
#else
    void* m_panelIo = nullptr;
    void* m_panel = nullptr;
#endif
};
} // namespace embedded
#endif