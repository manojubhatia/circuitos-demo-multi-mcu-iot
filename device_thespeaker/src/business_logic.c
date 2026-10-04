/**
 * Business Logic Implementation
 * Auto-generated for: thespeaker_3_boards
 * Generated: 
 * 
 * This file contains the business logic that connects components.
 * The rules here implement the user's application requirements.
 */

#include "business_logic.h"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "osal/osal_task.h"

// Periphery Handler for component access
#include "periphery_handler.h"

// Component includes
#include "digital_periphery.h"
#include "display_ui.h"

OSAL_LOG_MODULE_REGISTER(business_logic)

//=============================================================================
// Configuration
//=============================================================================

#define BUSINESS_LOGIC_INTERVAL_MS 100

//=============================================================================
// Global Variables for Business Logic State
//=============================================================================

// Timing state
static uint32_t s_last_process_time = 0;

//=============================================================================
// Public API
//=============================================================================

void business_logic_init(void) {
    OSAL_LOGI("Business logic initialized");
    OSAL_LOGI("  Processing interval: %d ms", BUSINESS_LOGIC_INTERVAL_MS);
    
    // Initialize any state needed for rules
    s_last_process_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void business_logic_process(void) {
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Rate limit processing
    if (now - s_last_process_time < BUSINESS_LOGIC_INTERVAL_MS) {
        return;
    }
    s_last_process_time = now;
    

}

uint32_t business_logic_get_interval_ms(void) {
    return BUSINESS_LOGIC_INTERVAL_MS;
}