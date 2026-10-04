/**
 * @file test_ldr_light_sensor.c
 * @brief Unit Tests for ldr_light_sensor

 * 
 * Tests for ldr_light_sensor
 */

#include "unity.h"
#include "ldr_light_sensor.h"

/* Include mocks for dependencies */
#define HAL_USE_MOCK 1
#include "hal_gpio_mock.h"
#include "hal_adc_mock.h"

/* Mock control functions */
extern void hal_gpio_mock_reset(void);
extern void hal_gpio_mock_set_level(hal_gpio_pin_t pin, bool level);
extern bool hal_gpio_mock_get_level(hal_gpio_pin_t pin);
extern void hal_gpio_mock_inject_error(hal_gpio_pin_t pin, hal_err_t error);
extern void hal_adc_mock_reset(void);
extern void hal_adc_mock_set_raw(hal_adc_channel_t channel, uint32_t value);
extern void hal_adc_mock_set_voltage(hal_adc_channel_t channel, float voltage_mv);

/* ============================================================================
 * Test Fixtures
 * ============================================================================ */

/* Sensor-specific test pins/channels */
#define TEST_LDR_LIGHT_SENSOR_PIN 4
#define TEST_LDR_LIGHT_SENSOR_ADC_CHANNEL 0

static ldr_light_sensor_t test_sensor;

/* ============================================================================
 * Setup / Teardown
 * ============================================================================ */

void setUp(void) {
    hal_gpio_mock_reset();
    hal_adc_mock_reset();
    memset(&test_sensor, 0, sizeof(test_sensor));
}

void tearDown(void) {
    ldr_light_sensor_deinit();
}

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

void test_ldr_light_sensor_init_success(void) {
    /* Given: Valid configuration */
    
    /* When: Initialize component */
    hal_err_t err = ldr_light_sensor_init();
    
    /* Then: Should succeed */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_ldr_light_sensor_init_invalid_params(void) {
    /* When: Initialize with NULL/invalid parameters */
    hal_err_t err = ldr_light_sensor_init(NULL);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

/* ============================================================================
 * Functional Tests
 * ============================================================================ */

/* Sensor Read Tests */

void test_ldr_light_sensor_read_success(void) {
    /* Given: Component initialized */
    hal_adc_mock_set_raw(TEST_LDR_LIGHT_SENSOR_ADC_CHANNEL, 2048);
    
    /* When: Read sensor */
    float result;
    hal_err_t err = ldr_light_sensor_read(&result);
    
    /* Then: Should return valid data */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_ldr_light_sensor_read_timeout(void) {
    /* Given: Component initialized, mock simulates timeout */
    ldr_light_sensor_init();
    hal_gpio_mock_inject_error(TEST_LDR_LIGHT_SENSOR_PIN, HAL_ERR_TIMEOUT);
    
    /* When: Read sensor */
    float result;
    hal_err_t err = ldr_light_sensor_read(&result);
    
    /* Then: Should return timeout error */
    TEST_ASSERT_EQUAL(HAL_ERR_TIMEOUT, err);
}

void test_ldr_light_sensor_read_range_min(void) {
    /* Given: Sensor returns minimum value */
    ldr_light_sensor_init();
    hal_adc_mock_set_raw(TEST_LDR_LIGHT_SENSOR_ADC_CHANNEL, 0);
    
    /* When: Read sensor */
    float result;
    hal_err_t err = ldr_light_sensor_read(&result);
    
    /* Then: Should return minimum valid value */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_ldr_light_sensor_read_range_max(void) {
    /* Given: Sensor returns maximum value */
    ldr_light_sensor_init();
    hal_adc_mock_set_raw(TEST_LDR_LIGHT_SENSOR_ADC_CHANNEL, 4095);
    
    /* When: Read sensor */
    float result;
    hal_err_t err = ldr_light_sensor_read(&result);
    
    /* Then: Should return maximum valid value */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

/* ============================================================================
 * Custom Tests from Configuration
 * ============================================================================ */

