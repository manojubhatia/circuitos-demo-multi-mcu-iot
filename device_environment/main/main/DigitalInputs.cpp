/**
 * @file DigitalInputs.cpp
 * @brief Implementation of Consolidated Digital Input Handler

 */

#include "DigitalInputs.hpp"

#include <cstdio>
#include "osal/osal.h"
#include "osal/osal_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
OSAL_LOG_MODULE_REGISTER(DigitalInputs);

#define DIGITAL_INPUTS_MILLIS() (uint32_t)(esp_timer_get_time() / 1000)
#define DIGITAL_INPUTS_LOG(fmt, ...) OSAL_LOGI(fmt, ##__VA_ARGS__)

// Static member definitions
bool DigitalInputs::_initialized = false;
DigitalInputs::InputState DigitalInputs::_states[DigitalInputs::INPUT_COUNT];
DigitalInputs::EventCallback DigitalInputs::_callback = nullptr;
QueueHandle_t DigitalInputs::_eventQueue = nullptr;
gpio_num_t DigitalInputs::_pins[DigitalInputs::INPUT_COUNT] = {};

bool DigitalInputs::init() {
    if (_initialized) return true;

    _eventQueue = xQueueCreate(16, sizeof(uint8_t));
    if (!_eventQueue) {
        DIGITAL_INPUTS_LOG("Failed to create event queue");
        return false;
    }

    gpio_install_isr_service(0);

    if (8 >= 0) {
        gpio_config_t io_conf_0 = {};
        io_conf_0.pin_bit_mask = (1ULL << 8);
        io_conf_0.mode = GPIO_MODE_INPUT;
        io_conf_0.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf_0.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_0.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf_0);
        gpio_isr_handler_add((gpio_num_t)8, gpio_isr_handler, (void*)(intptr_t)0);
        _pins[0] = (gpio_num_t)8;
    }
    if (13 >= 0) {
        gpio_config_t io_conf_1 = {};
        io_conf_1.pin_bit_mask = (1ULL << 13);
        io_conf_1.mode = GPIO_MODE_INPUT;
        io_conf_1.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf_1.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_1.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf_1);
        gpio_isr_handler_add((gpio_num_t)13, gpio_isr_handler, (void*)(intptr_t)1);
        _pins[1] = (gpio_num_t)13;
    }
    if (5 >= 0) {
        gpio_config_t io_conf_2 = {};
        io_conf_2.pin_bit_mask = (1ULL << 5);
        io_conf_2.mode = GPIO_MODE_INPUT;
        io_conf_2.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf_2.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_2.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf_2);
        gpio_isr_handler_add((gpio_num_t)5, gpio_isr_handler, (void*)(intptr_t)2);
        _pins[2] = (gpio_num_t)5;
    }
    if (9 >= 0) {
        gpio_config_t io_conf_3 = {};
        io_conf_3.pin_bit_mask = (1ULL << 9);
        io_conf_3.mode = GPIO_MODE_INPUT;
        io_conf_3.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf_3.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_3.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf_3);
        gpio_isr_handler_add((gpio_num_t)9, gpio_isr_handler, (void*)(intptr_t)3);
        _pins[3] = (gpio_num_t)9;
    }
    if (4 >= 0) {
        gpio_config_t io_conf_4 = {};
        io_conf_4.pin_bit_mask = (1ULL << 4);
        io_conf_4.mode = GPIO_MODE_INPUT;
        io_conf_4.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf_4.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_4.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf_4);
        gpio_isr_handler_add((gpio_num_t)4, gpio_isr_handler, (void*)(intptr_t)4);
        _pins[4] = (gpio_num_t)4;
    }
    if (9 >= 0) {
        gpio_config_t io_conf_5 = {};
        io_conf_5.pin_bit_mask = (1ULL << 9);
        io_conf_5.mode = GPIO_MODE_INPUT;
        io_conf_5.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf_5.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf_5.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&io_conf_5);
        gpio_isr_handler_add((gpio_num_t)9, gpio_isr_handler, (void*)(intptr_t)5);
        _pins[5] = (gpio_num_t)9;
    }

    _initialized = true;
    DIGITAL_INPUTS_LOG("Initialized %d inputs (interrupt-driven)", INPUT_COUNT);
    return true;
}

void DigitalInputs::update() {
    if (!_initialized) return;
    uint32_t now = DIGITAL_INPUTS_MILLIS();

    uint8_t input_id;
    while (xQueueReceive(_eventQueue, &input_id, 0) == pdTRUE) {
        if (input_id >= INPUT_COUNT) continue;

        InputState &s = _states[input_id];
        bool pressed = (gpio_get_level(_pins[input_id]) == 0);

        if ((now - s.lastChange) < BUTTON_DEBOUNCE_MS) continue;

        if (pressed != s.stable) {
            s.stable = pressed;
            s.lastChange = now;

            if (pressed) {
                s.pressStart = now;
                s.longPressFired = false;
                s.pendingEvent = EVENT_PRESSED;
                if (_callback) _callback((InputId)input_id, EVENT_PRESSED);
            } else {
                s.pendingEvent = EVENT_RELEASED;
                if (_callback) _callback((InputId)input_id, EVENT_RELEASED);
                if (!s.longPressFired && (now - s.pressStart) < BUTTON_LONG_PRESS_MS) {
                    if (_callback) _callback((InputId)input_id, EVENT_CLICK);
                }
            }
        }
    }

    for (int i = 0; i < INPUT_COUNT; i++) {
        InputState &s = _states[i];
        if (s.stable && !s.longPressFired && (now - s.pressStart) >= BUTTON_LONG_PRESS_MS) {
            s.longPressFired = true;
            s.pendingEvent = EVENT_LONG_PRESS;
            if (_callback) _callback((InputId)i, EVENT_LONG_PRESS);
        }
    }
}

bool DigitalInputs::isPressed(InputId id) {
    if (id >= INPUT_COUNT) return false;
    return _states[id].stable;
}

bool DigitalInputs::justPressed(InputId id) {
    if (id >= INPUT_COUNT) return false;
    if (_states[id].pendingEvent == EVENT_PRESSED) {
        _states[id].pendingEvent = EVENT_NONE;
        return true;
    }
    return false;
}

bool DigitalInputs::justReleased(InputId id) {
    if (id >= INPUT_COUNT) return false;
    if (_states[id].pendingEvent == EVENT_RELEASED) {
        _states[id].pendingEvent = EVENT_NONE;
        return true;
    }
    return false;
}

DigitalInputs::Event DigitalInputs::pollEvent(InputId id) {
    if (id >= INPUT_COUNT) return EVENT_NONE;
    Event evt = _states[id].pendingEvent;
    _states[id].pendingEvent = EVENT_NONE;
    return evt;
}

void DigitalInputs::setEventCallback(EventCallback callback) {
    _callback = callback;
}

int8_t DigitalInputs::getPin(InputId id) {
    static const int8_t pins[INPUT_COUNT] = {
        8,
        13,
        5,
        9,
        4,
        9
    };
    return (id < INPUT_COUNT) ? pins[id] : -1;
}

const char* DigitalInputs::getName(InputId id) {
    static const char* names[INPUT_COUNT] = {
        "Joystick Up",
        "Joystick Down",
        "Joystick Left",
        "Joystick Right",
        "Joystick Center/Press",
        "Boot Button"
    };
    return (id < INPUT_COUNT) ? names[id] : "unknown";
}

void IRAM_ATTR DigitalInputs::gpio_isr_handler(void* arg) {
    uint8_t input_id = (uint8_t)(intptr_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(_eventQueue, &input_id, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}
