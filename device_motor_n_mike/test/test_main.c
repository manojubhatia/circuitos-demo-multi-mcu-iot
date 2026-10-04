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

/* inmp441_i2s_mems_microphone tests */
extern void test_inmp441_i2s_mems_microphone_init_success(void);
extern void test_inmp441_i2s_mems_microphone_init_invalid_params(void);
extern void test_inmp441_i2s_mems_microphone_read_success(void);
extern void test_inmp441_i2s_mems_microphone_read_timeout(void);

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

    /* inmp441_i2s_mems_microphone Tests */
    printf("\n--- inmp441_i2s_mems_microphone Tests ---\n");
    RUN_TEST(test_inmp441_i2s_mems_microphone_init_success);
    RUN_TEST(test_inmp441_i2s_mems_microphone_init_invalid_params);
    RUN_TEST(test_inmp441_i2s_mems_microphone_read_success);
    RUN_TEST(test_inmp441_i2s_mems_microphone_read_timeout);

    
    printf("\n==========================================\n");
    
    return UNITY_END();
}