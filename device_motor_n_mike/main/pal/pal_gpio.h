/**
 * @file pal_gpio.h
 * @brief Protocol Abstraction Layer - GPIO Interface
 */
#ifndef PAL_GPIO_H
#define PAL_GPIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAL_GPIO_INPUT = 0,
    PAL_GPIO_OUTPUT,
    PAL_GPIO_INPUT_PULLUP,
    PAL_GPIO_INPUT_PULLDOWN,
    PAL_GPIO_OUTPUT_OD,
} pal_gpio_mode_t;

typedef enum {
    PAL_GPIO_INTR_DISABLE = 0,
    PAL_GPIO_INTR_RISING,
    PAL_GPIO_INTR_FALLING,
    PAL_GPIO_INTR_BOTH,
    PAL_GPIO_INTR_LOW,
    PAL_GPIO_INTR_HIGH,
} pal_gpio_intr_t;

typedef void (*pal_gpio_isr_t)(int pin, void *user_data);

int pal_gpio_init(int pin, pal_gpio_mode_t mode);
void pal_gpio_set(int pin, bool level);
bool pal_gpio_get(int pin);
void pal_gpio_toggle(int pin);
int pal_gpio_set_interrupt(int pin, pal_gpio_intr_t type, pal_gpio_isr_t isr, void *user_data);
void pal_gpio_remove_interrupt(int pin);

#ifdef __cplusplus
}
#endif

#endif /* PAL_GPIO_H */