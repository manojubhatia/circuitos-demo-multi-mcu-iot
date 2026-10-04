/**
 * @file msl_config.c
 * @brief MSL Configuration - Auto-generated subscriptions from VCD protocol_mapping
 *
 * Generated for MCU: environment
 * This file registers all protocol backends and creates subscriptions
 * based on the dataflow connections defined in the Visual Circuit Designer.
 */

#include "msl_config.h"
#include <string.h>


#include "osal/osal_log.h"
OSAL_LOG_MODULE_REGISTER(msl_config)
#define CFG_LOGI(fmt, ...) OSAL_LOGI(fmt, ##__VA_ARGS__)


/* ─── Application Handlers ─── */



/* ─── Initialization ─── */

void msl_config_init(void)
{
    CFG_LOGI("Initializing MSL with auto-generated subscriptions");

    /* 1. Initialize MSL core */
    msl_init();

    /* 2. Register protocol backends (only those used by this node) */













    /* 3. Create subscriptions from VCD protocol_mapping */



    CFG_LOGI("No auto-subscriptions configured (add protocol_mapping in VCD)");

}