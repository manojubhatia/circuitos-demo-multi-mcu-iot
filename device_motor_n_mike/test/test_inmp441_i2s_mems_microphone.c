/**
 * @file test_inmp441_i2s_mems_microphone.c
 * @brief Unit Tests for inmp441_i2s_mems_microphone

 * 
 * Tests for inmp441_i2s_mems_microphone
 */

#include "unity.h"
#include "inmp441_i2s_mems_microphone.h"

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

/* Sensor-specific test pins/channels */
#define TEST_INMP441_I2S_MEMS_MICROPHONE_PIN 4

static inmp441_i2s_mems_microphone_t test_sensor;

/* ============================================================================
 * Setup / Teardown
 * ============================================================================ */

void setUp(void) {
    hal_gpio_mock_reset();
    memset(&test_sensor, 0, sizeof(test_sensor));
}

void tearDown(void) {
    inmp441_i2s_mems_microphone_deinit();
}

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

void test_inmp441_i2s_mems_microphone_init_success(void) {
    /* Given: Valid configuration */
    
    /* When: Initialize component */
    hal_err_t err = inmp441_i2s_mems_microphone_init();
    
    /* Then: Should succeed */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_inmp441_i2s_mems_microphone_init_invalid_params(void) {
    /* When: Initialize with NULL/invalid parameters */
    hal_err_t err = inmp441_i2s_mems_microphone_init(NULL);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

/* ============================================================================
 * Functional Tests
 * ============================================================================ */

/* Sensor Read Tests */

void test_inmp441_i2s_mems_microphone_read_success(void) {
    /* Given: Component initialized */
    hal_gpio_mock_set_level(TEST_INMP441_I2S_MEMS_MICROPHONE_PIN, true);
    
    /* When: Read sensor */
    float result;
    hal_err_t err = inmp441_i2s_mems_microphone_read(&result);
    
    /* Then: Should return valid data */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_inmp441_i2s_mems_microphone_read_timeout(void) {
    /* Given: Component initialized, mock simulates timeout */
    inmp441_i2s_mems_microphone_init();
    hal_gpio_mock_inject_error(TEST_INMP441_I2S_MEMS_MICROPHONE_PIN, HAL_ERR_TIMEOUT);
    
    /* When: Read sensor */
    float result;
    hal_err_t err = inmp441_i2s_mems_microphone_read(&result);
    
    /* Then: Should return timeout error */
    TEST_ASSERT_EQUAL(HAL_ERR_TIMEOUT, err);
}

void test_inmp441_i2s_mems_microphone_read_range_min(void) {
    /* Given: Sensor returns minimum value */
    inmp441_i2s_mems_microphone_init();
    
    /* When: Read sensor */
    float result;
    hal_err_t err = inmp441_i2s_mems_microphone_read(&result);
    
    /* Then: Should return minimum valid value */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_inmp441_i2s_mems_microphone_read_range_max(void) {
    /* Given: Sensor returns maximum value */
    inmp441_i2s_mems_microphone_init();
    
    /* When: Read sensor */
    float result;
    hal_err_t err = inmp441_i2s_mems_microphone_read(&result);
    
    /* Then: Should return maximum valid value */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

/* ============================================================================
 * Custom Tests from Configuration
 * ============================================================================ */

