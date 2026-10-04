/**
 * @file periphery_registry.h
 * @brief Auto-generated Periphery Signal Registry

 * 
 * This file contains the registry of all periphery signals in the system,
 * auto-generated from the circuit design.
 */

#ifndef PERIPHERY_REGISTRY_H
#define PERIPHERY_REGISTRY_H

#include "periphery_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all periphery signals and register with the state manager
 * @return 0 on success
 */
int periphery_registry_init(void);

/**
 * @brief Get signal handle by component and signal name
 * @param component Component name (e.g., "dht22_sensor")
 * @param signal Signal name within component (e.g., "temperature")
 * @return Signal handle or NULL
 */
periphery_signal_handle_t periphery_registry_get_signal(const char* component, const char* signal);

/* ============================================================================
 * Auto-generated Signal Accessors
 * ============================================================================ */

/* --- ILI9341 TFT Display --- */

/**
 * @brief Set brightness for ili9341_tft_display
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_ili9341_tft_display_brightness(int32_t value) {
    return periphery_state_set_int("ili9341_tft_display/brightness", value);
}

/**
 * @brief Set power for ili9341_tft_display
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_ili9341_tft_display_power(bool value) {
    return periphery_state_set_bool("ili9341_tft_display/power", value);
}
/* --- RGB LED --- */

/**
 * @brief Set state for rgb_led
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_rgb_led_state(bool value) {
    return periphery_state_set_bool("rgb_led/state", value);
}

/**
 * @brief Set brightness for rgb_led
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_rgb_led_brightness(int32_t value) {
    return periphery_state_set_int("rgb_led/brightness", value);
}
/* --- LDR Light Sensor --- */

/**
 * @brief Get value from ldr_light_sensor
 * @return float value
 */
static inline float periphery_get_ldr_light_sensor_value(void) {
    return periphery_state_get_float("ldr_light_sensor/value");
}

/* --- MicroSD Card Slot --- */

/**
 * @brief Get value from microsd_card_slot
 * @return float value
 */
static inline float periphery_get_microsd_card_slot_value(void) {
    return periphery_state_get_float("microsd_card_slot/value");
}

/* --- Speaker Connector (8002A Amp) --- */

/**
 * @brief Get value from speaker_connector_8002a_amp
 * @return float value
 */
static inline float periphery_get_speaker_connector_8002a_amp_value(void) {
    return periphery_state_get_float("speaker_connector_8002a_amp/value");
}

/* --- Bluetooth Speaker --- */

/**
 * @brief Set state for bluetooth_speaker
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_bluetooth_speaker_state(bool value) {
    return periphery_state_set_bool("bluetooth_speaker/state", value);
}

#ifdef __cplusplus
}
#endif

#endif /* PERIPHERY_REGISTRY_H */