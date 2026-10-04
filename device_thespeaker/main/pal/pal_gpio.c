/**
 * @file pal_gpio.c
 * @brief PAL GPIO implementation for ESP-IDF
 */
#include "pal_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"

static pal_gpio_isr_t s_isr_handlers[48] = {NULL};
static void *s_isr_user_data[48] = {NULL};
static bool s_isr_service_installed = false;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    int pin = (int)(intptr_t)arg;
    if (s_isr_handlers[pin]) s_isr_handlers[pin](pin, s_isr_user_data[pin]);
}

int pal_gpio_init(int pin, pal_gpio_mode_t mode)
{
    gpio_config_t cfg = { .pin_bit_mask = (1ULL << pin), .intr_type = GPIO_INTR_DISABLE };
    switch (mode) {
        case PAL_GPIO_INPUT:         cfg.mode = GPIO_MODE_INPUT; cfg.pull_up_en = 0; cfg.pull_down_en = 0; break;
        case PAL_GPIO_OUTPUT:        cfg.mode = GPIO_MODE_OUTPUT; break;
        case PAL_GPIO_INPUT_PULLUP:  cfg.mode = GPIO_MODE_INPUT; cfg.pull_up_en = 1; break;
        case PAL_GPIO_INPUT_PULLDOWN:cfg.mode = GPIO_MODE_INPUT; cfg.pull_down_en = 1; break;
        case PAL_GPIO_OUTPUT_OD:     cfg.mode = GPIO_MODE_OUTPUT_OD; break;
    }
    return (gpio_config(&cfg) == ESP_OK) ? 0 : -1;
}

void pal_gpio_set(int pin, bool level) { gpio_set_level((gpio_num_t)pin, level ? 1 : 0); }
bool pal_gpio_get(int pin) { return gpio_get_level((gpio_num_t)pin) != 0; }
void pal_gpio_toggle(int pin) { pal_gpio_set(pin, !pal_gpio_get(pin)); }

int pal_gpio_set_interrupt(int pin, pal_gpio_intr_t type, pal_gpio_isr_t isr, void *user_data)
{
    if (!s_isr_service_installed) { gpio_install_isr_service(0); s_isr_service_installed = true; }
    s_isr_handlers[pin] = isr;
    s_isr_user_data[pin] = user_data;
    gpio_int_type_t intr = GPIO_INTR_DISABLE;
    switch (type) {
        case PAL_GPIO_INTR_RISING: intr = GPIO_INTR_POSEDGE; break;
        case PAL_GPIO_INTR_FALLING: intr = GPIO_INTR_NEGEDGE; break;
        case PAL_GPIO_INTR_BOTH: intr = GPIO_INTR_ANYEDGE; break;
        case PAL_GPIO_INTR_LOW: intr = GPIO_INTR_LOW_LEVEL; break;
        case PAL_GPIO_INTR_HIGH: intr = GPIO_INTR_HIGH_LEVEL; break;
        default: break;
    }
    gpio_set_intr_type((gpio_num_t)pin, intr);
    gpio_isr_handler_add((gpio_num_t)pin, gpio_isr_handler, (void *)(intptr_t)pin);
    return 0;
}

void pal_gpio_remove_interrupt(int pin) { gpio_isr_handler_remove((gpio_num_t)pin); s_isr_handlers[pin] = NULL; }