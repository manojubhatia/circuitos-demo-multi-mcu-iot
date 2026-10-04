/**
 * @file DigitalInputs.hpp
 * @brief Consolidated Digital Input Handler for GPIO Buttons

 *
 * ESP-IDF: Uses GPIO interrupts for instant button detection
 * Arduino: Uses polling-based detection in update() call
 */

#ifndef DIGITAL_INPUTS_HPP
#define DIGITAL_INPUTS_HPP

#include <cstdint>
#include <atomic>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "pal/pal_gpio.h"
#include "osal/osal.h"
#include "osal/osal_queue.h"

#ifndef BUTTON_DEBOUNCE_MS
#define BUTTON_DEBOUNCE_MS 30
#endif

#ifndef BUTTON_LONG_PRESS_MS
#define BUTTON_LONG_PRESS_MS 1000
#endif

class DigitalInputs {
public:
    enum InputId : uint8_t {
        JOYSTICK_UP = 0,
        JOYSTICK_DOWN = 1,
        JOYSTICK_LEFT = 2,
        JOYSTICK_RIGHT = 3,
        JOYSTICK_CENTER = 4,
        BOOT_BUTTON = 5,
        INPUT_COUNT
    };

    enum Event : uint8_t {
        EVENT_NONE = 0,
        EVENT_PRESSED,
        EVENT_RELEASED,
        EVENT_LONG_PRESS,
        EVENT_CLICK
    };

    using EventCallback = void (*)(InputId input, Event event);

    static bool init();
    static void update();
    static bool isPressed(InputId id);
    static bool justPressed(InputId id);
    static bool justReleased(InputId id);
    static Event pollEvent(InputId id);
    static void setEventCallback(EventCallback callback);
    static int8_t getPin(InputId id);
    static const char* getName(InputId id);

private:
    struct InputState {
        bool raw = false;
        bool stable = false;
        bool wasPressed = false;
        uint32_t lastChange = 0;
        uint32_t pressStart = 0;
        bool longPressFired = false;
        Event pendingEvent = EVENT_NONE;
    };

    static bool _initialized;
    static InputState _states[INPUT_COUNT];
    static EventCallback _callback;
    static QueueHandle_t _eventQueue;
    static gpio_num_t _pins[INPUT_COUNT];
    static void IRAM_ATTR gpio_isr_handler(void* arg);
};

#define JOYSTICK_UP_PRESSED()     DigitalInputs::isPressed(DigitalInputs::JOYSTICK_UP)
#define JOYSTICK_DOWN_PRESSED()   DigitalInputs::isPressed(DigitalInputs::JOYSTICK_DOWN)
#define JOYSTICK_LEFT_PRESSED()   DigitalInputs::isPressed(DigitalInputs::JOYSTICK_LEFT)
#define JOYSTICK_RIGHT_PRESSED()  DigitalInputs::isPressed(DigitalInputs::JOYSTICK_RIGHT)
#define JOYSTICK_CENTER_PRESSED() DigitalInputs::isPressed(DigitalInputs::JOYSTICK_CENTER)

#endif // DIGITAL_INPUTS_HPP