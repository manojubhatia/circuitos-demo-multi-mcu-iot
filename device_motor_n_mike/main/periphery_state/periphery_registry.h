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

/* --- INMP441 I2S MEMS Microphone --- */

/**
 * @brief Get value from inmp441_i2s_mems_microphone
 * @return float value
 */
static inline float periphery_get_inmp441_i2s_mems_microphone_value(void) {
    return periphery_state_get_float("inmp441_i2s_mems_microphone/value");
}

#ifdef __cplusplus
}
#endif

#endif /* PERIPHERY_REGISTRY_H */