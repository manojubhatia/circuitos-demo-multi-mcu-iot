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

/* st7735s_lcd_display tests */
/* joystick_up tests */
/* joystick_down tests */
/* joystick_left tests */
/* joystick_right tests */
/* joystick_center_press tests */
/* boot_button tests */
/* aht10_temperature_humidity_sensor tests */
extern void test_aht10_temperature_humidity_sensor_init_success(void);
extern void test_aht10_temperature_humidity_sensor_init_invalid_params(void);
extern void test_aht10_temperature_humidity_sensor_read_success(void);
extern void test_aht10_temperature_humidity_sensor_read_timeout(void);

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

    /* st7735s_lcd_display Tests */
    printf("\n--- st7735s_lcd_display Tests ---\n");

    /* joystick_up Tests */
    printf("\n--- joystick_up Tests ---\n");

    /* joystick_down Tests */
    printf("\n--- joystick_down Tests ---\n");

    /* joystick_left Tests */
    printf("\n--- joystick_left Tests ---\n");

    /* joystick_right Tests */
    printf("\n--- joystick_right Tests ---\n");

    /* joystick_center_press Tests */
    printf("\n--- joystick_center_press Tests ---\n");

    /* boot_button Tests */
    printf("\n--- boot_button Tests ---\n");

    /* aht10_temperature_humidity_sensor Tests */
    printf("\n--- aht10_temperature_humidity_sensor Tests ---\n");
    RUN_TEST(test_aht10_temperature_humidity_sensor_init_success);
    RUN_TEST(test_aht10_temperature_humidity_sensor_init_invalid_params);
    RUN_TEST(test_aht10_temperature_humidity_sensor_read_success);
    RUN_TEST(test_aht10_temperature_humidity_sensor_read_timeout);

    
    printf("\n==========================================\n");
    
    return UNITY_END();
}