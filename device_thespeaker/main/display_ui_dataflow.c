/**
 * @file display_ui_dataflow.c
 * @brief Dataflow-UI bidirectional handler implementation
 * @note Auto-generated - connects dataflow variables to LVGL and vice versa
 */

#include "display_ui_dataflow.h"
#include "display_ui.h"
#include "esp_lvgl_port.h"
#include <string.h>
#include <strings.h>

// ========== OUTPUT BINDINGS (Dataflow -> UI) ==========

#define DISPLAY_UI_OUTPUT_BINDING_COUNT 10

typedef struct {
    const char* var_id;
    void (*update_func)(float value);
} display_ui_output_binding_t;

static const display_ui_output_binding_t s_output_bindings[DISPLAY_UI_OUTPUT_BINDING_COUNT] = {
    { "environment_aht10_temperature", update_lbl_val_widget_auto_0 },
    { "environment_aht10_humidity", update_lbl_val_widget_auto_1 },
    { "motor_n_mike_inmp441_audio_samples", update_lbl_val_widget_auto_2 },
    { "motor_n_mike_inmp441_audio_level", update_lbl_val_widget_auto_3 },
    { "motor_n_mike_inmp441_peak_amplitude", update_lbl_val_widget_auto_4 },
    { "motor_n_mike_inmp441_rms_level", update_lbl_val_widget_auto_5 },
    { "motor_n_mike_inmp441_is_clipping", update_lbl_val_widget_auto_6 },
    { "thespeaker_bluetoothspeaker_connected", update_lbl_val_widget_auto_7 },
    { "thespeaker_bluetoothspeaker_device_name", update_lbl_val_widget_auto_8 },
    { "thespeaker_bluetoothspeaker_rssi", update_lbl_val_widget_auto_9 },
};

// ========== INPUT BINDINGS (UI -> Dataflow) ==========

#define DISPLAY_UI_INPUT_BINDING_COUNT 0

typedef struct {
    const char* var_id;
    const char* widget_type;
} display_ui_input_binding_t;

static const display_ui_input_binding_t* s_input_bindings __attribute__((unused)) = NULL;

// Input callback (set by user to forward to dataflow)
static display_ui_input_callback_t s_input_callback = NULL;

void display_ui_dataflow_init(void) {
    // Bindings are statically defined, no runtime init needed
    // This function can be extended for dynamic registration
}

// ========== OUTPUT FUNCTIONS ==========

bool display_ui_dataflow_update(const char* var_id, const void* value, size_t size) {
    if (!display_ui_is_ready() || var_id == NULL || value == NULL || DISPLAY_UI_OUTPUT_BINDING_COUNT == 0) {
        return false;
    }
    
    // Strip "board_name::" prefix if present (main.cpp sends "env::sig", bindings use "sig")
    const char* sep = strstr(var_id, "::");
    const char* lookup_id = sep ? (sep + 2) : var_id;
    
    bool matched = false;
    float float_value = 0.0f;
    bool value_decoded = false;

    // Iterate ALL bindings (a variable may drive multiple widgets,
    // e.g. a value label AND a trend chart)
    for (int i = 0; i < DISPLAY_UI_OUTPUT_BINDING_COUNT; i++) {
        // Match logic: frame descriptor field names may differ from UI binding
        // var_ids in case (descriptors are lowercase) and may have a board-name
        // prefix (e.g. "environment_aht10_..." vs binding "aht10_...").
        const char* binding_id = s_output_bindings[i].var_id;
        bool match = (strcmp(binding_id, lookup_id) == 0);
        if (!match) match = (strcasecmp(binding_id, lookup_id) == 0);
        if (!match) {
            // Try stripping successive underscore-delimited prefixes from
            // lookup_id (e.g. "environment_aht10_..." -> "aht10_...")
            const char* suffix = lookup_id;
            while (!match && (suffix = strchr(suffix, '_')) != NULL) {
                suffix++;  // skip past the '_'
                if (strcasecmp(binding_id, suffix) == 0) {
                    match = true;
                }
            }
        }
        if (match) {
            if (!value_decoded) {
                if (size == sizeof(float)) {
                    memcpy(&float_value, value, sizeof(float));
                } else if (size == sizeof(int32_t)) {
                    int32_t int_val;
                    memcpy(&int_val, value, sizeof(int32_t));
                    float_value = (float)int_val;
                } else if (size == sizeof(int16_t)) {
                    int16_t int_val;
                    memcpy(&int_val, value, sizeof(int16_t));
                    float_value = (float)int_val;
                } else if (size == sizeof(uint8_t)) {
                    uint8_t int_val;
                    memcpy(&int_val, value, sizeof(uint8_t));
                    float_value = (float)int_val;
                }
                value_decoded = true;
            }

            if (s_output_bindings[i].update_func != NULL) {
                if (lvgl_port_lock(50)) {
                    s_output_bindings[i].update_func(float_value);
                    lvgl_port_unlock();
                }
            }
            matched = true;
        }
    }

    return matched;
}

int display_ui_dataflow_get_output_binding_count(void) {
    return DISPLAY_UI_OUTPUT_BINDING_COUNT;
}

// ========== INPUT FUNCTIONS ==========

void display_ui_set_input_callback(display_ui_input_callback_t callback) {
    s_input_callback = callback;
}

void display_ui_input_send(const char* var_id, int32_t value) {
    if (var_id == NULL || var_id[0] == '\0') {
        return;  // No target variable configured
    }
    
    if (s_input_callback != NULL) {
        s_input_callback(var_id, value);
    }
    
    // TODO: If no callback set, can directly call dataflow_send() here
}

void display_ui_input_send_float(const char* var_id, float value) {
    // Convert to int32_t (can be extended for proper float handling)
    display_ui_input_send(var_id, (int32_t)value);
}

int display_ui_dataflow_get_input_binding_count(void) {
    return DISPLAY_UI_INPUT_BINDING_COUNT;
}
