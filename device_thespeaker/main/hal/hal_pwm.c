/**
 * @file hal_pwm_esp.c
 * @brief Hardware Abstraction Layer - PWM ESP-IDF Implementation (LEDC)

 */

#include "hal_pwm.h"

#if HAL_PLATFORM_ESP_IDF

#include "driver/ledc.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "hal_pwm";

typedef struct {
    bool configured;
    hal_pwm_config_t config;
    ledc_channel_t ledc_channel;
    ledc_timer_t ledc_timer;
} pwm_channel_state_t;

static pwm_channel_state_t s_channels[HAL_PWM_MAX_CHANNELS];
static bool s_initialized = false;

hal_err_t hal_pwm_init(void) {
    if (s_initialized) return HAL_OK;
    memset(s_channels, 0, sizeof(s_channels));
    s_initialized = true;
    return HAL_OK;
}

hal_err_t hal_pwm_deinit(void) {
    for (int i = 0; i < HAL_PWM_MAX_CHANNELS; i++) {
        if (s_channels[i].configured) hal_pwm_stop(i);
    }
    s_initialized = false;
    return HAL_OK;
}

hal_err_t hal_pwm_configure(const hal_pwm_config_t* config) {
    if (!config || config->channel < 0 || config->channel >= HAL_PWM_MAX_CHANNELS) {
        return HAL_ERR_INVALID_ARG;
    }
    if (!s_initialized) hal_pwm_init();

    int ch = config->channel;
    ledc_timer_t timer = (ledc_timer_t)(ch / 2);   /* 2 channels per timer */
    ledc_channel_t ledc_ch = (ledc_channel_t)(ch % LEDC_CHANNEL_MAX);

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)config->resolution_bits,
        .timer_num = timer,
        .freq_hz = config->frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }

    ledc_channel_config_t ch_cfg = {
        .gpio_num = config->pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = ledc_ch,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = timer,
        .duty = 0,
        .hpoint = 0,
    };
    err = ledc_channel_config(&ch_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(err));
        return HAL_ERR_FAILED;
    }

    s_channels[ch].configured = true;
    s_channels[ch].config = *config;
    s_channels[ch].ledc_channel = ledc_ch;
    s_channels[ch].ledc_timer = timer;

    ESP_LOGI(TAG, "PWM ch%d: pin=%d, freq=%lu Hz, %d-bit",
             ch, (int)config->pin, (unsigned long)config->frequency_hz, config->resolution_bits);
    return HAL_OK;
}

hal_err_t hal_pwm_set_duty(hal_pwm_channel_t channel, uint32_t duty) {
    if (channel < 0 || channel >= HAL_PWM_MAX_CHANNELS || !s_channels[channel].configured) {
        return HAL_ERR_INVALID_ARG;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, s_channels[channel].ledc_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, s_channels[channel].ledc_channel);
    return HAL_OK;
}

hal_err_t hal_pwm_set_duty_percent(hal_pwm_channel_t channel, float percent) {
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    uint32_t max = hal_pwm_get_max_duty(channel);
    uint32_t duty = (uint32_t)((percent / 100.0f) * (float)max);
    return hal_pwm_set_duty(channel, duty);
}

hal_err_t hal_pwm_set_frequency(hal_pwm_channel_t channel, uint32_t frequency_hz) {
    if (channel < 0 || channel >= HAL_PWM_MAX_CHANNELS || !s_channels[channel].configured) {
        return HAL_ERR_INVALID_ARG;
    }
    ledc_set_freq(LEDC_LOW_SPEED_MODE, s_channels[channel].ledc_timer, frequency_hz);
    s_channels[channel].config.frequency_hz = frequency_hz;
    return HAL_OK;
}

hal_err_t hal_pwm_stop(hal_pwm_channel_t channel) {
    return hal_pwm_set_duty(channel, 0);
}

uint32_t hal_pwm_get_max_duty(hal_pwm_channel_t channel) {
    if (channel < 0 || channel >= HAL_PWM_MAX_CHANNELS || !s_channels[channel].configured) {
        return 255;
    }
    return (1U << s_channels[channel].config.resolution_bits) - 1;
}

#endif /* HAL_PLATFORM_ESP_IDF */