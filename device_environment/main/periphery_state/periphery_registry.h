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

/* --- ST7735S LCD Display --- */

/**
 * @brief Set brightness for st7735s_lcd_display
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_st7735s_lcd_display_brightness(int32_t value) {
    return periphery_state_set_int("st7735s_lcd_display/brightness", value);
}

/**
 * @brief Set power for st7735s_lcd_display
 * @param value Value to set
 * @return 0 on success
 */
static inline int periphery_set_st7735s_lcd_display_power(bool value) {
    return periphery_state_set_bool("st7735s_lcd_display/power", value);
}
/* --- AHT10 Temperature/Humidity Sensor --- */

/**
 * @brief Get temperature from aht10_temperature_humidity_sensor
 * @return float value
 */
static inline float periphery_get_aht10_temperature_humidity_sensor_temperature(void) {
    return periphery_state_get_float("aht10_temperature_humidity_sensor/temperature");
}

/**
 * @brief Get humidity from aht10_temperature_humidity_sensor
 * @return float value
 */
static inline float periphery_get_aht10_temperature_humidity_sensor_humidity(void) {
    return periphery_state_get_float("aht10_temperature_humidity_sensor/humidity");
}

#ifdef __cplusplus
}
#endif

#endif /* PERIPHERY_REGISTRY_H */