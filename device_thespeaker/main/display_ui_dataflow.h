/**
 * @file display_ui_dataflow.h
 * @brief Dataflow-UI bidirectional handler - Auto-generated
 * @note Connects dataflow variables to LVGL display and vice versa
 */

#ifndef DISPLAY_UI_DATAFLOW_H
#define DISPLAY_UI_DATAFLOW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "display_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Signal that display UI is ready for dataflow updates
 */
static inline void display_ui_dataflow_set_ready(void) {
    display_ui_set_ready();
}

/**
 * @brief Initialize dataflow-UI bindings
 * Call this after dataflow_exchange_init() and display_ui_init()
 */
void display_ui_dataflow_init(void);

// ========== OUTPUT: Dataflow -> UI ==========

/**
 * @brief Process incoming dataflow packet and update UI widgets
 * @param var_id Variable identifier string
 * @param value Pointer to value data
 * @param size Size of value in bytes
 * @return true if handled, false if variable not bound to UI
 */
bool display_ui_dataflow_update(const char* var_id, const void* value, size_t size);

/**
 * @brief Get number of output (display) bindings
 */
int display_ui_dataflow_get_output_binding_count(void);

// ========== INPUT: UI -> Dataflow ==========

/**
 * @brief Send input value from UI widget to dataflow system
 * @param var_id Target variable identifier
 * @param value Value to send (integer)
 * 
 * Call this from UI event callbacks (button press, slider change, etc.)
 */
void display_ui_input_send(const char* var_id, int32_t value);

/**
 * @brief Send float input value from UI widget to dataflow system
 * @param var_id Target variable identifier
 * @param value Float value to send
 */
void display_ui_input_send_float(const char* var_id, float value);

/**
 * @brief Set callback for handling UI input events
 * @param callback Function to call when UI sends input (var_id, value)
 * 
 * The callback should forward to dataflow_send() or handle locally.
 */
typedef void (*display_ui_input_callback_t)(const char* var_id, int32_t value);
void display_ui_set_input_callback(display_ui_input_callback_t callback);

/**
 * @brief Get number of input (touch control) bindings
 */
int display_ui_dataflow_get_input_binding_count(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_UI_DATAFLOW_H
