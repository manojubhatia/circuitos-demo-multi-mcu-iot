/**
 * @file pal_pwm.h
 * @brief Protocol Abstraction Layer - PWM Interface
 */
#ifndef PAL_PWM_H
#define PAL_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int pal_pwm_init(int pin, uint32_t frequency_hz, uint8_t resolution_bits);
int pal_pwm_set_duty(int pin, uint32_t duty);
int pal_pwm_set_duty_percent(int pin, float percent);
int pal_pwm_set_frequency(int pin, uint32_t frequency_hz);
void pal_pwm_stop(int pin);

#ifdef __cplusplus
}
#endif

#endif /* PAL_PWM_H */