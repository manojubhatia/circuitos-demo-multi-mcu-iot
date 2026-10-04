/**
 * @file hooks.c
 * @brief Hook instances and default implementations
 * @note Generated file - defines global hook instances.
 */

#include "lifecycle_hooks.h"
#include "component_hooks.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Global Hook Instances
 * ═══════════════════════════════════════════════════════════════════════════ */

// Lifecycle hooks instance
lifecycle_hooks_t g_lifecycle_hooks = {0};

// Component hooks instance
component_hooks_t g_component_hooks = {0};

/* ═══════════════════════════════════════════════════════════════════════════
 * Hook System Initialization
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialize all hook systems
 * Call this before any hooks are registered
 */
void hooks_system_init(void) {
    lifecycle_hooks_init();
    component_hooks_init();
}