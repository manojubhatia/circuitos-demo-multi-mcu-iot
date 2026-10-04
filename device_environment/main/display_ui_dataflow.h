/**
 * @file display_ui_dataflow.h
 * @brief Raw Display UI Dataflow Handler - Auto-generated
 */

#ifndef DISPLAY_UI_DATAFLOW_H
#define DISPLAY_UI_DATAFLOW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "display_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void display_ui_dataflow_set_ready(void) {
    display_ui_set_ready();
}

static inline void display_ui_dataflow_update(const char* var_id, const void* value, size_t size) {
    if (!display_ui_is_ready() || !var_id || !value || size < sizeof(float)) return;
    float fval;
    memcpy(&fval, value, sizeof(float));

    // Strip "board_name::" prefix if present (main.cpp sends "env::sig", bindings use "sig")
    const char* sep = strstr(var_id, "::");
    const char* lookup_id = sep ? (sep + 2) : var_id;

    if (strcmp(lookup_id, "environment_aht10_temperature") == 0) { update_val_widget_auto_0(fval); }
    if (strcmp(lookup_id, "environment_aht10_humidity") == 0) { update_val_widget_auto_1(fval); }
    if (strcmp(lookup_id, "motor_n_mike_inmp441_audio_samples") == 0) { update_val_widget_auto_2(fval); }
    if (strcmp(lookup_id, "motor_n_mike_inmp441_audio_level") == 0) { update_val_widget_auto_3(fval); }
    if (strcmp(lookup_id, "motor_n_mike_inmp441_peak_amplitude") == 0) { update_val_widget_auto_4(fval); }
    if (strcmp(lookup_id, "motor_n_mike_inmp441_rms_level") == 0) { update_val_widget_auto_5(fval); }
    if (strcmp(lookup_id, "motor_n_mike_inmp441_is_clipping") == 0) { update_val_widget_auto_6(fval); }
    if (strcmp(lookup_id, "thespeaker_bluetoothspeaker_connected") == 0) { update_val_widget_auto_7(fval); }
    if (strcmp(lookup_id, "thespeaker_bluetoothspeaker_device_name") == 0) { update_val_widget_auto_8(fval); }
    if (strcmp(lookup_id, "thespeaker_bluetoothspeaker_rssi") == 0) { update_val_widget_auto_9(fval); }

}

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_UI_DATAFLOW_H */
