/**
 * Business Logic Header
 * Auto-generated for: thespeaker_3_boards
 * Generated: 
 * 
 * This file contains the business logic that connects components.
 * The rules here implement the user's application requirements.
 * 
 * Component implementations are in separate files - this file only
 * contains the "glue code" that connects them.
 */

#ifndef BUSINESS_LOGIC_H
#define BUSINESS_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize business logic
 * Call this after all components are initialized
 */
void business_logic_init(void);

/**
 * Process business logic
 * Call this periodically from the main loop or a dedicated task
 */
void business_logic_process(void);

/**
 * Get the recommended processing interval in milliseconds
 */
uint32_t business_logic_get_interval_ms(void);

#ifdef __cplusplus
}
#endif

#endif // BUSINESS_LOGIC_H