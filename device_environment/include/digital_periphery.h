/**
 * Digital Periphery Module - Auto-generated from Jinja template
 * 
 * This module provides digital GPIO operations with automatic
 * registration to the Periphery Handler for centralized management.
 * 
 * Generated for: thespeaker_3_boards
 */

#ifndef DIGITAL_PERIPHERY_H
#define DIGITAL_PERIPHERY_H

#include <stdbool.h>
#include <stdint.h>
#include "osal/osal_types.h"
#include "periphery_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Configuration Structure
//=============================================================================

/**
 * @brief Digital periphery pin configuration
 */
typedef struct {
    const char *name;       // Logical name (e.g., "led", "button")
    uint8_t pin;            // GPIO pin number
    bool is_output;         // true = output, false = input
    bool active_low;        // Invert logic (true = active when LOW)
    bool pull_up;           // Enable internal pull-up (for inputs)
    bool pull_down;         // Enable internal pull-down (for inputs)
    bool initial_state;     // Initial state for outputs
} digital_periphery_config_t;

//=============================================================================
// API Functions
//=============================================================================

/**
 * @brief Initialize digital periphery and register with Periphery Handler
 * 
 * This function:
 * 1. Configures GPIO pins
 * 2. Registers each pin as a Periphery Handler channel
 * 3. Sets up read/write callbacks for the Periphery Handler
 * 
 * @param configs Array of pin configurations
 * @param count Number of configurations
 * @return ESP_OK on success
 */
esp_err_t digital_periphery_register_with_handler(const digital_periphery_config_t *configs, size_t count);

/**
 * @brief Write to a digital output by name (via Periphery Handler)
 * @param name Logical name of the pin
 * @param state true = HIGH (or LOW if active_low)
 * @return ESP_OK on success
 */
esp_err_t digital_periphery_write(const char *name, bool state);

/**
 * @brief Read from a digital input by name (via Periphery Handler)
 * @param name Logical name of the pin
 * @param state Pointer to store the read state
 * @return ESP_OK on success
 */
esp_err_t digital_periphery_read(const char *name, bool *state);

/**
 * @brief Toggle a digital output by name
 * @param name Logical name of the pin
 * @return ESP_OK on success
 */
esp_err_t digital_periphery_toggle(const char *name);

/**
 * @brief Get the GPIO pin number for a named pin
 * @param name Logical name of the pin
 * @return GPIO pin number, or -1 if not found
 */
int digital_periphery_get_pin(const char *name);

#ifdef __cplusplus
}
#endif

#endif // DIGITAL_PERIPHERY_H
