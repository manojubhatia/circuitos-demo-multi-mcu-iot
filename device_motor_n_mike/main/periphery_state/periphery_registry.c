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
        .name = "inmp441_i2s_mems_microphone/value",
        .component = "inmp441_i2s_mems_microphone",
        .type = PERIPHERY_TYPE_FLOAT,
        .direction = PERIPHERY_DIR_OUTPUT,
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