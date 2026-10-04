/**
 * @file pal_adc.c
 * @brief PAL ADC implementation for ESP-IDF
 */
#include "pal_adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "pal_adc";
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;

int pal_adc_init(int channel, pal_adc_atten_t atten, pal_adc_width_t width)
{
    if (!adc_handle) {
        adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
        esp_err_t err = adc_oneshot_new_unit(&init_cfg, &adc_handle);
        if (err != ESP_OK) { ESP_LOGE(TAG, "Unit init failed"); return -1; }
    }
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = (adc_atten_t)atten,
        .bitwidth = (width == 12) ? ADC_BITWIDTH_12 : ADC_BITWIDTH_DEFAULT,
    };
    return (adc_oneshot_config_channel(adc_handle, channel, &chan_cfg) == ESP_OK) ? 0 : -1;
}

int pal_adc_read_raw(int channel)
{
    int raw = 0;
    if (adc_oneshot_read(adc_handle, channel, &raw) != ESP_OK) return -1;
    return raw;
}

float pal_adc_read_voltage(int channel)
{
    int raw = pal_adc_read_raw(channel);
    if (raw < 0) return -1.0f;
    if (cali_handle) {
        int mv = 0;
        adc_cali_raw_to_voltage(cali_handle, raw, &mv);
        return mv / 1000.0f;
    }
    return (raw / 4095.0f) * 3.3f;
}

int pal_adc_read_averaged(int channel, int samples)
{
    int sum = 0;
    for (int i = 0; i < samples; i++) {
        int raw = pal_adc_read_raw(channel);
        if (raw < 0) return -1;
        sum += raw;
    }
    return sum / samples;
}