/**
 * @file pal_adc.h
 * @brief Protocol Abstraction Layer - ADC Interface
 */
#ifndef PAL_ADC_H
#define PAL_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAL_ADC_ATTEN_0DB = 0,
    PAL_ADC_ATTEN_2_5DB,
    PAL_ADC_ATTEN_6DB,
    PAL_ADC_ATTEN_11DB,
} pal_adc_atten_t;

typedef enum {
    PAL_ADC_WIDTH_9BIT = 9,
    PAL_ADC_WIDTH_10BIT = 10,
    PAL_ADC_WIDTH_11BIT = 11,
    PAL_ADC_WIDTH_12BIT = 12,
    PAL_ADC_WIDTH_13BIT = 13,
} pal_adc_width_t;

int pal_adc_init(int channel, pal_adc_atten_t atten, pal_adc_width_t width);
int pal_adc_read_raw(int channel);
float pal_adc_read_voltage(int channel);
int pal_adc_read_averaged(int channel, int samples);

#ifdef __cplusplus
}
#endif

#endif /* PAL_ADC_H */