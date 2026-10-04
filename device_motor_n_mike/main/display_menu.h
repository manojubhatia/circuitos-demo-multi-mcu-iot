#ifndef DISPLAY_MENU_H
#define DISPLAY_MENU_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    MENU_INPUT_UP = 0,
    MENU_INPUT_DOWN,
    MENU_INPUT_LEFT,
    MENU_INPUT_RIGHT,
    MENU_INPUT_SELECT,
    MENU_INPUT_BACK,
} menu_input_t;
static inline bool display_menu_init(void) { return true; }
static inline void display_menu_update(void) {}
static inline void display_menu_input(menu_input_t input) { (void)input; }
static inline void display_set_status(const char *status) { (void)status; }
static inline void display_set_wifi_status(bool connected, const char *ssid, const char *ip, int rssi) {
    (void)connected; (void)ssid; (void)ip; (void)rssi;
}
static inline void display_menu_show_progress(const char *msg, int pct) { (void)msg; (void)pct; }
static inline void display_menu_show_message(const char *msg, int line) { (void)msg; (void)line; }
static inline void display_update_sensor(const char *name, float val, const char *unit) { (void)name; (void)val; (void)unit; }
#ifdef __cplusplus
}
#endif
#endif
