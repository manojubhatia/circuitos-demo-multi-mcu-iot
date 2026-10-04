/**
 * @file msl_config.h
 * @brief MSL Configuration - Auto-generated protocol selection and subscriptions
 *
 * Generated from VCD protocol_mapping for MCU: environment
 * Only protocols used in this node are included.
 */

#ifndef MSL_CONFIG_H
#define MSL_CONFIG_H

#include "msl.h"
#include "msl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol backends used by this node */














/**
 * Initialize all MSL backends used by this node and register subscriptions.
 * Call after WiFi/network is ready.
 */
void msl_config_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MSL_CONFIG_H */