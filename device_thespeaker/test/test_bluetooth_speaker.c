/**
 * @file test_bluetooth_speaker.c
 * @brief Unit Tests for bluetooth_speaker

 * 
 * Tests for bluetooth_speaker
 */

#include "unity.h"
#include "bluetooth_speaker.h"

/* Include mocks for dependencies */
#define HAL_USE_MOCK 1
#include "hal_gpio_mock.h"

/* Mock control functions */
extern void hal_gpio_mock_reset(void);
extern void hal_gpio_mock_set_level(hal_gpio_pin_t pin, bool level);
extern bool hal_gpio_mock_get_level(hal_gpio_pin_t pin);
extern void hal_gpio_mock_inject_error(hal_gpio_pin_t pin, hal_err_t error);

/* ============================================================================
 * Test Fixtures
 * ============================================================================ */

/* Actuator-specific test pins */
#define TEST_BLUETOOTH_SPEAKER_PIN 4

static bluetooth_speaker_t test_actuator;

/* ============================================================================
 * Setup / Teardown
 * ============================================================================ */

void setUp(void) {
    hal_gpio_mock_reset();
    memset(&test_actuator, 0, sizeof(test_actuator));
}

void tearDown(void) {
    bluetooth_speaker_deinit();
}

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

void test_bluetooth_speaker_init_success(void) {
    /* Given: Valid configuration */
    
    /* When: Initialize component */
    hal_err_t err = bluetooth_speaker_init();
    
    /* Then: Should succeed */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_bluetooth_speaker_init_invalid_params(void) {
    /* When: Initialize with NULL/invalid parameters */
    hal_err_t err = bluetooth_speaker_init(NULL);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

/* ============================================================================
 * Functional Tests
 * ============================================================================ */

/* Actuator Control Tests */

void test_bluetooth_speaker_set_state_on(void) {
    /* Given: Component initialized */
    bluetooth_speaker_init();
    
    /* When: Set state to ON */
    hal_err_t err = bluetooth_speaker_set(true);
    
    /* Then: Should succeed and GPIO should be set */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_TRUE(hal_gpio_mock_get_level(TEST_BLUETOOTH_SPEAKER_PIN));
}

void test_bluetooth_speaker_set_state_off(void) {
    /* Given: Component initialized and ON */
    bluetooth_speaker_init();
    bluetooth_speaker_set(true);
    
    /* When: Set state to OFF */
    hal_err_t err = bluetooth_speaker_set(false);
    
    /* Then: Should succeed and GPIO should be cleared */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_FALSE(hal_gpio_mock_get_level(TEST_BLUETOOTH_SPEAKER_PIN));
}

/* ============================================================================
 * Custom Tests from Configuration
 * ============================================================================ */

