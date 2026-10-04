/**
 * @file pal_pwm.c
 * @brief PAL PWM implementation for ESP-IDF (LEDC)
 */
#include "pal_pwm.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "pal_pwm";
static uint8_t s_next_channel = 0;
static uint8_t s_pin_channel[48] = {0};
static uint8_t s_resolution[48] = {0};

int pal_pwm_init(int pin, uint32_t frequency_hz, uint8_t resolution_bits)
{
    if (s_next_channel >= LEDC_CHANNEL_MAX) { ESP_LOGE(TAG, "No free LEDC channels"); return -1; }
    uint8_t ch = s_next_channel++;
    s_pin_channel[pin] = ch;
    s_resolution[pin] = resolution_bits;

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = (ledc_timer_t)(ch / 2),
        .duty_resolution = resolution_bits,
        .freq_hz = frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t)ch,
        .timer_sel = (ledc_timer_t)(ch / 2),
        .gpio_num = pin,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);
    ESP_LOGI(TAG, "PWM pin%d ch%d @ %luHz %d-bit", pin, ch, (unsigned long)frequency_hz, resolution_bits);
    return 0;
}

int pal_pwm_set_duty(int pin, uint32_t duty)
{
    uint8_t ch = s_pin_channel[pin];
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch);
    return 0;
}

int pal_pwm_set_duty_percent(int pin, float percent)
{
    uint32_t max_duty = (1U << s_resolution[pin]) - 1;
    uint32_t duty = (uint32_t)(percent / 100.0f * max_duty);
    return pal_pwm_set_duty(pin, duty);
}

int pal_pwm_set_frequency(int pin, uint32_t frequency_hz)
{
    uint8_t ch = s_pin_channel[pin];
    ledc_set_freq(LEDC_LOW_SPEED_MODE, (ledc_timer_t)(ch / 2), frequency_hz);
    return 0;
}

void pal_pwm_stop(int pin) { pal_pwm_set_duty(pin, 0); }