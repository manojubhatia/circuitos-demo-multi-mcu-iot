/**
 * @file test_hal_gpio.c
 * @brief Unit Tests for HAL GPIO

 */

#include "unity.h"
#include "hal_gpio.h"

/* For mock testing */
#define HAL_USE_MOCK 1

/* Mock control functions */
extern void hal_gpio_mock_reset(void);
extern void hal_gpio_mock_set_level(hal_gpio_pin_t pin, bool level);
extern bool hal_gpio_mock_get_level(hal_gpio_pin_t pin);
extern void hal_gpio_mock_expect_init(hal_gpio_pin_t pin, hal_gpio_mode_t mode, hal_gpio_pull_t pull);
extern bool hal_gpio_mock_verify(void);
extern void hal_gpio_mock_inject_error(hal_gpio_pin_t pin, hal_err_t error);
extern void hal_gpio_mock_trigger_interrupt(hal_gpio_pin_t pin);
extern bool hal_gpio_mock_is_configured(hal_gpio_pin_t pin);
extern hal_gpio_mode_t hal_gpio_mock_get_mode(hal_gpio_pin_t pin);

/* ============================================================================
 * Test Fixtures
 * ============================================================================ */

#define TEST_PIN_OUTPUT  2
#define TEST_PIN_INPUT   4
#define TEST_PIN_INTR    5

static volatile int isr_call_count = 0;
static volatile void* isr_received_arg = NULL;

static void test_isr_handler(void* arg) {
    isr_call_count++;
    isr_received_arg = arg;
}

void setUp(void) {
    hal_gpio_mock_reset();
    isr_call_count = 0;
    isr_received_arg = NULL;
}

void tearDown(void) {
    hal_gpio_deinit();
}

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

void test_hal_gpio_init(void) {
    /* Test: GPIO subsystem initializes successfully */
    hal_err_t err = hal_gpio_init();
    TEST_ASSERT_EQUAL(HAL_OK, err);
    
    /* Test: Re-initialization is idempotent */
    err = hal_gpio_init();
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_hal_gpio_deinit(void) {
    /* Given: GPIO is initialized */
    hal_gpio_init();
    
    /* When: Deinitialize */
    hal_err_t err = hal_gpio_deinit();
    
    /* Then: Should succeed */
    TEST_ASSERT_EQUAL(HAL_OK, err);
}

void test_hal_gpio_configure_output(void) {
    /* Given: Expectation set */
    hal_gpio_mock_expect_init(TEST_PIN_OUTPUT, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
    
    /* When: Configure as output */
    hal_gpio_config_t config = HAL_GPIO_CONFIG_DEFAULT();
    config.pin = TEST_PIN_OUTPUT;
    config.mode = HAL_GPIO_MODE_OUTPUT;
    config.pull = HAL_GPIO_PULL_NONE;
    
    hal_err_t err = hal_gpio_configure(&config);
    
    /* Then: Should succeed and match expectation */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_TRUE(hal_gpio_mock_verify());
    TEST_ASSERT_TRUE(hal_gpio_mock_is_configured(TEST_PIN_OUTPUT));
    TEST_ASSERT_EQUAL(HAL_GPIO_MODE_OUTPUT, hal_gpio_mock_get_mode(TEST_PIN_OUTPUT));
}

void test_hal_gpio_configure_input_pullup(void) {
    /* When: Configure as input with pull-up */
    hal_err_t err = hal_gpio_set_mode(TEST_PIN_INPUT, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);
    
    /* Then: Should succeed */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_EQUAL(HAL_GPIO_MODE_INPUT, hal_gpio_mock_get_mode(TEST_PIN_INPUT));
}

void test_hal_gpio_configure_invalid_pin(void) {
    /* Given: Invalid pin number */
    hal_gpio_config_t config = HAL_GPIO_CONFIG_DEFAULT();
    config.pin = -1;
    
    /* When: Attempt to configure */
    hal_err_t err = hal_gpio_configure(&config);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

void test_hal_gpio_configure_null_config(void) {
    /* When: Pass NULL config */
    hal_err_t err = hal_gpio_configure(NULL);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

/* ============================================================================
 * Level Control Tests
 * ============================================================================ */

void test_hal_gpio_set_level(void) {
    /* Given: Pin configured as output */
    hal_gpio_set_mode(TEST_PIN_OUTPUT, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
    
    /* When: Set level high */
    hal_err_t err = hal_gpio_set_level(TEST_PIN_OUTPUT, true);
    
    /* Then: Should succeed and level should be high */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_TRUE(hal_gpio_mock_get_level(TEST_PIN_OUTPUT));
    
    /* When: Set level low */
    err = hal_gpio_set_level(TEST_PIN_OUTPUT, false);
    
    /* Then: Should succeed and level should be low */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_FALSE(hal_gpio_mock_get_level(TEST_PIN_OUTPUT));
}

void test_hal_gpio_set_level_invalid_pin(void) {
    /* When: Set level on invalid pin */
    hal_err_t err = hal_gpio_set_level(-1, true);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

void test_hal_gpio_get_level(void) {
    /* Given: Pin configured as input with known level */
    hal_gpio_set_mode(TEST_PIN_INPUT, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_NONE);
    hal_gpio_mock_set_level(TEST_PIN_INPUT, true);
    
    /* When: Get level */
    bool level = hal_gpio_get_level(TEST_PIN_INPUT);
    
    /* Then: Should return correct level */
    TEST_ASSERT_TRUE(level);
    
    /* Given: Level changed */
    hal_gpio_mock_set_level(TEST_PIN_INPUT, false);
    
    /* When: Get level again */
    level = hal_gpio_get_level(TEST_PIN_INPUT);
    
    /* Then: Should return new level */
    TEST_ASSERT_FALSE(level);
}

void test_hal_gpio_toggle(void) {
    /* Given: Pin configured as output, initially low */
    hal_gpio_set_mode(TEST_PIN_OUTPUT, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
    hal_gpio_set_level(TEST_PIN_OUTPUT, false);
    
    /* When: Toggle */
    hal_err_t err = hal_gpio_toggle(TEST_PIN_OUTPUT);
    
    /* Then: Should be high */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_TRUE(hal_gpio_mock_get_level(TEST_PIN_OUTPUT));
    
    /* When: Toggle again */
    err = hal_gpio_toggle(TEST_PIN_OUTPUT);
    
    /* Then: Should be low */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_FALSE(hal_gpio_mock_get_level(TEST_PIN_OUTPUT));
}

void test_hal_gpio_toggle_invalid_pin(void) {
    /* When: Toggle invalid pin */
    hal_err_t err = hal_gpio_toggle(-1);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

/* ============================================================================
 * Interrupt Tests
 * ============================================================================ */

void test_hal_gpio_interrupt(void) {
    /* Given: Pin configured with interrupt */
    hal_gpio_set_mode(TEST_PIN_INTR, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);
    
    int test_arg = 42;
    hal_err_t err = hal_gpio_set_interrupt(
        TEST_PIN_INTR,
        HAL_GPIO_INTR_FALLING,
        test_isr_handler,
        &test_arg
    );
    TEST_ASSERT_EQUAL(HAL_OK, err);
    
    err = hal_gpio_intr_enable(TEST_PIN_INTR);
    TEST_ASSERT_EQUAL(HAL_OK, err);
    
    /* When: Trigger interrupt */
    hal_gpio_mock_trigger_interrupt(TEST_PIN_INTR);
    
    /* Then: ISR should be called with correct argument */
    TEST_ASSERT_EQUAL(1, isr_call_count);
    TEST_ASSERT_EQUAL_PTR(&test_arg, isr_received_arg);
}

void test_hal_gpio_interrupt_disabled(void) {
    /* Given: Pin configured with interrupt, but disabled */
    hal_gpio_set_mode(TEST_PIN_INTR, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);
    hal_gpio_set_interrupt(TEST_PIN_INTR, HAL_GPIO_INTR_FALLING, test_isr_handler, NULL);
    hal_gpio_intr_disable(TEST_PIN_INTR);
    
    /* When: Trigger interrupt */
    hal_gpio_mock_trigger_interrupt(TEST_PIN_INTR);
    
    /* Then: ISR should NOT be called */
    TEST_ASSERT_EQUAL(0, isr_call_count);
}

void test_hal_gpio_interrupt_invalid_pin(void) {
    /* When: Set interrupt on invalid pin */
    hal_err_t err = hal_gpio_set_interrupt(-1, HAL_GPIO_INTR_RISING, test_isr_handler, NULL);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}

/* ============================================================================
 * Error Injection Tests
 * ============================================================================ */

void test_hal_gpio_error_injection(void) {
    /* Given: Error injected for specific pin */
    hal_gpio_mock_inject_error(TEST_PIN_OUTPUT, HAL_ERR_FAILED);
    
    /* When: Try to set level */
    hal_err_t err = hal_gpio_set_level(TEST_PIN_OUTPUT, true);
    
    /* Then: Should return injected error */
    TEST_ASSERT_EQUAL(HAL_ERR_FAILED, err);
}

/* ============================================================================
 * Reset Tests
 * ============================================================================ */

void test_hal_gpio_reset(void) {
    /* Given: Pin is configured */
    hal_gpio_set_mode(TEST_PIN_OUTPUT, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
    hal_gpio_set_level(TEST_PIN_OUTPUT, true);
    TEST_ASSERT_TRUE(hal_gpio_mock_is_configured(TEST_PIN_OUTPUT));
    
    /* When: Reset pin */
    hal_err_t err = hal_gpio_reset(TEST_PIN_OUTPUT);
    
    /* Then: Should succeed and pin should be unconfigured */
    TEST_ASSERT_EQUAL(HAL_OK, err);
    TEST_ASSERT_FALSE(hal_gpio_mock_is_configured(TEST_PIN_OUTPUT));
}

void test_hal_gpio_reset_invalid_pin(void) {
    /* When: Reset invalid pin */
    hal_err_t err = hal_gpio_reset(-1);
    
    /* Then: Should return error */
    TEST_ASSERT_EQUAL(HAL_ERR_INVALID_ARG, err);
}