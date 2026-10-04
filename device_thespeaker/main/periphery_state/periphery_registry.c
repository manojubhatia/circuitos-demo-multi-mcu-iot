/**
 * @file periphery_registry.c
 * @brief Auto-generated IO Signal Registry Implementation

 */

#include "periphery_registry.h"
#include "../middleware/middleware.h"
#include <string.h>
#include <stdio.h>

/* Signal definitions */
typedef struct {
    const char* name;
    const char* component;
    periphery_type_t type;
    periphery_direction_t direction;
    const char* topic;  /* Optional middleware topic */
} periphery_signal_def_t;

static const periphery_signal_def_t s_signal_defs[] = {
    {
        .name = "ili9341_tft_display/brightness",
        .component = "ili9341_tft_display",
        .type = PERIPHERY_TYPE_INT32,
        .direction = PERIPHERY_DIR_INPUT,
        .topic = NULL
    },
    {
        .name = "ili9341_tft_display/power",
        .component = "ili9341_tft_display",
        .type = PERIPHERY_TYPE_BOOL,
        .direction = PERIPHERY_DIR_INPUT,
        .topic = NULL
    },
    {
        .name = "rgb_led/state",
        .component = "rgb_led",
        .type = PERIPHERY_TYPE_BOOL,
        .direction = PERIPHERY_DIR_INPUT,
        .topic = NULL
    },
    {
        .name = "rgb_led/brightness",
        .component = "rgb_led",
        .type = PERIPHERY_TYPE_INT32,
        .direction = PERIPHERY_DIR_INPUT,
        .topic = NULL
    },
    {
        .name = "ldr_light_sensor/value",
        .component = "ldr_light_sensor",
        .type = PERIPHERY_TYPE_FLOAT,
        .direction = PERIPHERY_DIR_OUTPUT,
        .topic = NULL
    },
    {
        .name = "microsd_card_slot/value",
        .component = "microsd_card_slot",
        .type = PERIPHERY_TYPE_FLOAT,
        .direction = PERIPHERY_DIR_OUTPUT,
        .topic = NULL
    },
    {
        .name = "speaker_connector_8002a_amp/value",
        .component = "speaker_connector_8002a_amp",
        .type = PERIPHERY_TYPE_FLOAT,
        .direction = PERIPHERY_DIR_OUTPUT,
        .topic = NULL
    },
    {
        .name = "bluetooth_speaker/state",
        .component = "bluetooth_speaker",
        .type = PERIPHERY_TYPE_BOOL,
        .direction = PERIPHERY_DIR_INPUT,
        .topic = NULL
    },
};
#define SIGNAL_COUNT (sizeof(s_signal_defs) / sizeof(s_signal_defs[0]))

int periphery_registry_init(void) {
    int ret;
    
    /* Initialize IO state manager */
    ret = periphery_state_init();
    if (ret != 0) return ret;
    
    /* Initialize middleware */
    ret = mw_init();
    if (ret != 0) return ret;
    
    /* Register all signals */
    for (size_t i = 0; i < SIGNAL_COUNT; i++) {
        const periphery_signal_def_t* def = &s_signal_defs[i];
        
        periphery_signal_handle_t sig = periphery_state_register(
            def->name,
            def->type,
            def->direction,
            def->component
        );
        
        if (!sig) {
            return -1;
        }
        
        /* Setup middleware auto-publish/subscribe if topic specified */
        if (def->topic) {
            if (def->direction == PERIPHERY_DIR_OUTPUT || def->direction == PERIPHERY_DIR_BIDIRECTIONAL) {
                periphery_state_enable_autopublish(def->name, def->topic);
            }
            if (def->direction == PERIPHERY_DIR_INPUT || def->direction == PERIPHERY_DIR_BIDIRECTIONAL) {
                periphery_state_enable_autosubscribe(def->name, def->topic);
            }
        }
    }
    
    (void)ret;  /* Suppress warning if no signals */
    return 0;
}

periphery_signal_handle_t periphery_registry_get_signal(const char* component, const char* signal) {
    char full_name[128];
    snprintf(full_name, sizeof(full_name), "%s/%s", component, signal);
    return periphery_state_find(full_name);
}