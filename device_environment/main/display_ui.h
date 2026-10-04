/**
 * @file display_ui.h
 * @brief Raw Display UI (no LVGL) - Auto-generated
 * @resolution 160x80
 * @note Uses direct display driver calls for rendering.
 */

#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_WIDTH  160
#define DISPLAY_HEIGHT 80
#define DISPLAY_RENDER_MODE_RAW 1

void display_ui_init(void);
void display_ui_refresh(void);
void display_ui_tick(void);

bool display_ui_is_ready(void);
void display_ui_set_ready(void);

/**
 * @brief Bind the display driver instance for raw rendering.
 * Call this from main.cpp after creating the display driver.
 * @param driver Pointer to the display driver object (cast to void*)
 */
void display_ui_bind_driver(void* driver);
void update_val_widget_auto_0(float value);
void update_val_widget_auto_1(float value);
void update_val_widget_auto_2(float value);
void update_val_widget_auto_3(float value);
void update_val_widget_auto_4(float value);
void update_val_widget_auto_5(float value);
void update_val_widget_auto_6(float value);
void update_val_widget_auto_7(float value);
void update_val_widget_auto_8(float value);
void update_val_widget_auto_9(float value);
void display_ui_tick(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_UI_H
