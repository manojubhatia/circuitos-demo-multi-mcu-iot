/**
 * @file hal_pwm.h
 * @brief Hardware Abstraction Layer - PWM Interface

 */

#ifndef HAL_PWM_H
#define HAL_PWM_H

#include "hal_types.h"
#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    hal_pwm_channel_t channel;
    hal_gpio_pin_t pin;
    uint32_t frequency_hz;
    uint8_t resolution_bits;    /* 1-16 bits */
} hal_pwm_config_t;

#define HAL_PWM_CONFIG_DEFAULT() { \
    .channel = HAL_PWM_CHANNEL_INVALID, \
    .pin = HAL_GPIO_PIN_INVALID,        \
    .frequency_hz = HAL_PWM_DEFAULT_FREQ_HZ, \
    .resolution_bits = HAL_PWM_DEFAULT_RESOLUTION, \
}

hal_err_t hal_pwm_init(void);
hal_err_t hal_pwm_deinit(void);

hal_err_t hal_pwm_configure(const hal_pwm_config_t* config);

/**
 * @brief Set PWM duty cycle (0 to max value for resolution)
 * @param channel PWM channel
 * @param duty Raw duty value (0 to 2^resolution - 1)
 */
hal_err_t hal_pwm_set_duty(hal_pwm_channel_t channel, uint32_t duty);

/**
 * @brief Set PWM duty cycle as percentage (0.0 to 100.0)
 */
hal_err_t hal_pwm_set_duty_percent(hal_pwm_channel_t channel, float percent);

/**
 * @brief Set PWM frequency (may affect all channels on same timer)
 */
hal_err_t hal_pwm_set_frequency(hal_pwm_channel_t channel, uint32_t frequency_hz);

/**
 * @brief Stop PWM output (set duty to 0)
 */
hal_err_t hal_pwm_stop(hal_pwm_channel_t channel);

/**
 * @brief Get maximum duty value for channel's resolution
 */
uint32_t hal_pwm_get_max_duty(hal_pwm_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif /* HAL_PWM_H */