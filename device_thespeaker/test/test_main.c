/**
 * @file test_main.c
 * @brief Unit Test Runner

 * 
 * Main entry point for running all unit tests.
 * Uses Unity test framework.
 */

#include "unity.h"
#include <stdio.h>

/* ============================================================================
 * Test Suite Declarations
 * ============================================================================ */

/* ili9341_tft_display tests */
/* xpt2046_touch_controller tests */
/* rgb_led tests */
extern void test_rgb_led_init_success(void);
extern void test_rgb_led_set_state_on(void);
extern void test_rgb_led_set_state_off(void);
/* ldr_light_sensor tests */
extern void test_ldr_light_sensor_init_success(void);
extern void test_ldr_light_sensor_init_invalid_params(void);
extern void test_ldr_light_sensor_read_success(void);
extern void test_ldr_light_sensor_read_timeout(void);
/* microsd_card_slot tests */
extern void test_microsd_card_slot_init_success(void);
extern void test_microsd_card_slot_init_invalid_params(void);
extern void test_microsd_card_slot_read_success(void);
extern void test_microsd_card_slot_read_timeout(void);
/* speaker_connector_8002a_amp tests */
extern void test_speaker_connector_8002a_amp_init_success(void);
extern void test_speaker_connector_8002a_amp_init_invalid_params(void);
extern void test_speaker_connector_8002a_amp_read_success(void);
extern void test_speaker_connector_8002a_amp_read_timeout(void);
/* bluetooth_speaker tests */
extern void test_bluetooth_speaker_init_success(void);
extern void test_bluetooth_speaker_set_state_on(void);
extern void test_bluetooth_speaker_set_state_off(void);

/* HAL tests */
extern void test_hal_gpio_init(void);
extern void test_hal_gpio_set_level(void);
extern void test_hal_gpio_get_level(void);
extern void test_hal_gpio_toggle(void);
extern void test_hal_gpio_interrupt(void);

/* ============================================================================
 * Unity Setup/Teardown
 * ============================================================================ */

void setUp(void) {
    /* Reset all mocks before each test */
#ifdef HAL_USE_MOCK
    extern void hal_gpio_mock_reset(void);
    hal_gpio_mock_reset();
#endif
}

void tearDown(void) {
    /* Cleanup after each test */
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

int main(void) {
    UNITY_BEGIN();
    
    printf("\n");
    printf("==========================================\n");
    printf("  theSpeaker (3 boards) - Unit Tests\n");
    printf("==========================================\n\n");
    
    /* HAL GPIO Tests */
    printf("--- HAL GPIO Tests ---\n");
    RUN_TEST(test_hal_gpio_init);
    RUN_TEST(test_hal_gpio_set_level);
    RUN_TEST(test_hal_gpio_get_level);
    RUN_TEST(test_hal_gpio_toggle);
    RUN_TEST(test_hal_gpio_interrupt);

    /* ili9341_tft_display Tests */
    printf("\n--- ili9341_tft_display Tests ---\n");

    /* xpt2046_touch_controller Tests */
    printf("\n--- xpt2046_touch_controller Tests ---\n");

    /* rgb_led Tests */
    printf("\n--- rgb_led Tests ---\n");
    RUN_TEST(test_rgb_led_init_success);
    RUN_TEST(test_rgb_led_set_state_on);
    RUN_TEST(test_rgb_led_set_state_off);

    /* ldr_light_sensor Tests */
    printf("\n--- ldr_light_sensor Tests ---\n");
    RUN_TEST(test_ldr_light_sensor_init_success);
    RUN_TEST(test_ldr_light_sensor_init_invalid_params);
    RUN_TEST(test_ldr_light_sensor_read_success);
    RUN_TEST(test_ldr_light_sensor_read_timeout);

    /* microsd_card_slot Tests */
    printf("\n--- microsd_card_slot Tests ---\n");
    RUN_TEST(test_microsd_card_slot_init_success);
    RUN_TEST(test_microsd_card_slot_init_invalid_params);
    RUN_TEST(test_microsd_card_slot_read_success);
    RUN_TEST(test_microsd_card_slot_read_timeout);

    /* speaker_connector_8002a_amp Tests */
    printf("\n--- speaker_connector_8002a_amp Tests ---\n");
    RUN_TEST(test_speaker_connector_8002a_amp_init_success);
    RUN_TEST(test_speaker_connector_8002a_amp_init_invalid_params);
    RUN_TEST(test_speaker_connector_8002a_amp_read_success);
    RUN_TEST(test_speaker_connector_8002a_amp_read_timeout);

    /* bluetooth_speaker Tests */
    printf("\n--- bluetooth_speaker Tests ---\n");
    RUN_TEST(test_bluetooth_speaker_init_success);
    RUN_TEST(test_bluetooth_speaker_set_state_on);
    RUN_TEST(test_bluetooth_speaker_set_state_off);

    
    printf("\n==========================================\n");
    
    return UNITY_END();
}