/**
 * @file lifecycle_hooks.h
 * @brief Application lifecycle hooks for theSpeaker (3 boards)
 * @note Generated file - regenerated when components change.
 *       User implementations in user/ directory are preserved.
 */

#ifndef LIFECYCLE_HOOKS_H
#define LIFECYCLE_HOOKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Hook Function Types
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef void (*hook_fn_void_t)(void);
typedef void (*hook_fn_tick_t)(uint32_t tick_ms);
typedef void (*hook_fn_error_t)(int error_code, const char* message);
typedef void (*hook_fn_data_t)(const uint8_t* data, size_t len);

/* ═══════════════════════════════════════════════════════════════════════════
 * Lifecycle Hooks Structure
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    // Initialization hooks
    hook_fn_void_t on_pre_init;           // Before any initialization
    hook_fn_void_t on_post_init;          // After all components initialized
    
    // Main loop hooks
    hook_fn_void_t on_loop_start;         // At start of each loop iteration
    hook_fn_void_t on_loop_end;           // At end of each loop iteration
    hook_fn_tick_t on_tick;               // Called every tick with elapsed ms
    
    // Lifecycle events
    hook_fn_void_t on_idle;               // Called during idle time
    hook_fn_void_t on_sleep;              // Before entering sleep mode
    hook_fn_void_t on_wake;               // After waking from sleep
    hook_fn_void_t on_shutdown;           // Before shutdown
    
    // Error handling
    hook_fn_error_t on_error;             // On any error
    hook_fn_void_t on_watchdog;           // Watchdog trigger
    
    // Communication
    hook_fn_data_t on_data_received;      // External data received
    hook_fn_data_t on_data_send;          // Before sending data
    
} lifecycle_hooks_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * Global Hooks Instance
 * ═══════════════════════════════════════════════════════════════════════════ */

extern lifecycle_hooks_t g_lifecycle_hooks;

/* ═══════════════════════════════════════════════════════════════════════════
 * Hook Invocation Macros
 * ═══════════════════════════════════════════════════════════════════════════ */

// Call a void hook if registered
#define CALL_HOOK_VOID(name) \
    do { if (g_lifecycle_hooks.name) g_lifecycle_hooks.name(); } while(0)

// Call a hook with one argument
#define CALL_HOOK_1(name, arg1) \
    do { if (g_lifecycle_hooks.name) g_lifecycle_hooks.name(arg1); } while(0)

// Call a hook with two arguments
#define CALL_HOOK_2(name, arg1, arg2) \
    do { if (g_lifecycle_hooks.name) g_lifecycle_hooks.name((arg1), (arg2)); } while(0)

// Call error hook
#define CALL_ERROR_HOOK(code, msg) \
    do { if (g_lifecycle_hooks.on_error) g_lifecycle_hooks.on_error((code), (msg)); } while(0)

// Call data hooks
#define CALL_DATA_RECEIVED_HOOK(data, len) \
    do { if (g_lifecycle_hooks.on_data_received) g_lifecycle_hooks.on_data_received((data), (len)); } while(0)

#define CALL_DATA_SEND_HOOK(data, len) \
    do { if (g_lifecycle_hooks.on_data_send) g_lifecycle_hooks.on_data_send((data), (len)); } while(0)

/* ═══════════════════════════════════════════════════════════════════════════
 * Hook Registration Helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * Initialize hooks structure to NULL
 */
static inline void lifecycle_hooks_init(void) {
    memset(&g_lifecycle_hooks, 0, sizeof(g_lifecycle_hooks));
}

/**
 * Register an init hook
 */
static inline void register_init_hook(hook_fn_void_t hook) {
    g_lifecycle_hooks.on_post_init = hook;
}

/**
 * Register a loop start hook
 */
static inline void register_loop_start_hook(hook_fn_void_t hook) {
    g_lifecycle_hooks.on_loop_start = hook;
}

/**
 * Register a loop end hook
 */
static inline void register_loop_end_hook(hook_fn_void_t hook) {
    g_lifecycle_hooks.on_loop_end = hook;
}

/**
 * Register an error hook
 */
static inline void register_error_hook(hook_fn_error_t hook) {
    g_lifecycle_hooks.on_error = hook;
}

#ifdef __cplusplus
}
#endif

#endif // LIFECYCLE_HOOKS_H