/**
 * @file pal_i2s.h
 * @brief Protocol Abstraction Layer - I2S Audio Interface
 */
#ifndef PAL_I2S_H
#define PAL_I2S_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAL_I2S_MODE_MASTER_TX = 0,
    PAL_I2S_MODE_MASTER_RX,
    PAL_I2S_MODE_MASTER_DUPLEX,
} pal_i2s_mode_t;

typedef enum {
    PAL_I2S_BITS_16 = 16,
    PAL_I2S_BITS_24 = 24,
    PAL_I2S_BITS_32 = 32,
} pal_i2s_bits_t;

typedef struct {
    pal_i2s_mode_t mode;
    uint32_t sample_rate;
    pal_i2s_bits_t bits_per_sample;
    uint8_t channels;
    int bck_pin;
    int ws_pin;
    int data_out_pin;
    int data_in_pin;
    uint16_t dma_buf_count;
    uint16_t dma_buf_len;
} pal_i2s_config_t;

int pal_i2s_init(int port, const pal_i2s_config_t *config);
void pal_i2s_deinit(int port);
int pal_i2s_write(int port, const void *data, size_t len, uint32_t timeout_ms);
int pal_i2s_read(int port, void *data, size_t len, uint32_t timeout_ms);
int pal_i2s_set_sample_rate(int port, uint32_t rate);

#ifdef __cplusplus
}
#endif

#endif /* PAL_I2S_H */