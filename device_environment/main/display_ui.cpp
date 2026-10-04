/**
 * @file display_ui.cpp
 * @brief Raw Display UI Implementation (no LVGL)
 * @note Calls the actual display driver (ST7735) for rendering.
 *       Uses OSAL mutex to protect SPI bus access from concurrent tasks.
 */

#include "display_ui.h"
#include "displays/ST7735.hpp"
#include <cstring>
#include <cstdio>
#include "osal/osal_sync.h"
#include "osal/osal_types.h"

using namespace embedded;

static bool s_display_ui_ready = false;
static ST7735* s_display_ptr = nullptr;
static osal_mutex_handle_t s_display_mutex = nullptr;
static bool s_refresh_pending = false;

bool display_ui_is_ready(void) { return s_display_ui_ready; }
void display_ui_set_ready(void) { s_display_ui_ready = true; }

void display_ui_bind_driver(void* driver) {
    s_display_ptr = static_cast<ST7735*>(driver);
    if (!s_display_mutex) {
        osal_mutex_create(&s_display_mutex);
    }
}

void display_ui_tick(void) {
    if (!s_refresh_pending || !s_display_ptr) return;
    if (s_display_mutex && osal_mutex_lock(s_display_mutex, 50) == OSAL_OK) {
        s_refresh_pending = false;
        display_ui_refresh();
        osal_mutex_unlock(s_display_mutex);
    }
}

static char s_buf_widget_auto_0[32] = "--";
static char s_buf_widget_auto_1[32] = "--";
static char s_buf_widget_auto_2[32] = "--";
static char s_buf_widget_auto_3[32] = "--";
static char s_buf_widget_auto_4[32] = "--";
static char s_buf_widget_auto_5[32] = "--";
static char s_buf_widget_auto_6[32] = "--";
static char s_buf_widget_auto_7[32] = "--";
static char s_buf_widget_auto_8[32] = "--";
static char s_buf_widget_auto_9[32] = "--";

void display_ui_init(void) {
    if (!s_display_ptr) return;
    s_display_ptr->fillRect(0, 0, 160, 80, 0x001F);
    display_ui_refresh();
}

void display_ui_refresh(void) {
    if (!s_display_ptr) return;
    auto* disp = s_display_ptr;
    disp->setTextSize(1); disp->setCursor(2, 2); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_0);
    disp->setTextSize(1); disp->setCursor(54, 2); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_1);
    disp->setTextSize(1); disp->setCursor(106, 2); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_2);
    disp->setTextSize(1); disp->setCursor(2, 21); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_3);
    disp->setTextSize(1); disp->setCursor(54, 21); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_4);
    disp->setTextSize(1); disp->setCursor(106, 21); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_5);
    disp->setTextSize(1); disp->setCursor(2, 40); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_6);
    disp->setTextSize(1); disp->setCursor(54, 40); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_7);
    disp->setTextSize(1); disp->setCursor(106, 40); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_8);
    disp->setTextSize(1); disp->setCursor(2, 59); disp->setTextColor(0x07E0, 0x001F); disp->print(s_buf_widget_auto_9);
}

void update_val_widget_auto_0(float value) {
    snprintf(s_buf_widget_auto_0, sizeof(s_buf_widget_auto_0), "%.1fcelsius", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_1(float value) {
    snprintf(s_buf_widget_auto_1, sizeof(s_buf_widget_auto_1), "%.1fpercent", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_2(float value) {
    snprintf(s_buf_widget_auto_2, sizeof(s_buf_widget_auto_2), "%.1f", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_3(float value) {
    snprintf(s_buf_widget_auto_3, sizeof(s_buf_widget_auto_3), "%.1fdB", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_4(float value) {
    snprintf(s_buf_widget_auto_4, sizeof(s_buf_widget_auto_4), "%.1f", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_5(float value) {
    snprintf(s_buf_widget_auto_5, sizeof(s_buf_widget_auto_5), "%.1f", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_6(float value) {
    snprintf(s_buf_widget_auto_6, sizeof(s_buf_widget_auto_6), "%.1f", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_7(float value) {
    snprintf(s_buf_widget_auto_7, sizeof(s_buf_widget_auto_7), "%.1f", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_8(float value) {
    snprintf(s_buf_widget_auto_8, sizeof(s_buf_widget_auto_8), "%.1f", value);
    s_refresh_pending = true;
}

void update_val_widget_auto_9(float value) {
    snprintf(s_buf_widget_auto_9, sizeof(s_buf_widget_auto_9), "%.1f", value);
    s_refresh_pending = true;
}

