# Architecture

> This document describes the enforced software architecture of this CircuitOS-generated project.

## Design Philosophy

CircuitOS Studio enforces a strict separation of concerns through a **4-layer architecture**
that ensures portability, testability, and maintainability. The architecture is not optional —
it is structurally enforced by the code generation engine. No developer can bypass it.

## Layer Diagram

```
╔═══════════════════════════════════════════════════════════════════╗
║                        APPLICATION LAYER                         ║
║  • State machines and task orchestration                         ║
║  • Business logic and decision making                            ║
║  • Uses only Middleware + OSAL APIs                              ║
╠═══════════════════════════════════════════════════════════════════╣
║                       MIDDLEWARE LAYER                            ║
║  • Event bus (pub/sub) for decoupled communication               ║
║  • Telemetry sender — formats and dispatches sensor data         ║
║  • DataFlow engine — cross-MCU variable synchronization          ║
║  • Signal conditioning, filtering, aggregation                   ║
╠═══════════════════════════════════════════════════════════════════╣
║                    COMPONENT DRIVER LAYER                        ║
║  • C++ classes wrapping each sensor/actuator                     ║
║  • Clean init() / read() / write() / deinit() interfaces        ║
║  • Uses PAL for all hardware access                              ║
╠═══════════════════════════════════════════════════════════════════╣
║                   OSAL (OS Abstraction Layer)                    ║
║  • Task creation, scheduling, priorities                         ║
║  • Timers, delays, logging, assertions                           ║
║  • Mutexes, semaphores, queues                                   ║
║  • Maps to FreeRTOS on ESP-IDF, k_thread on Zephyr              ║
╠═══════════════════════════════════════════════════════════════════╣
║                   PAL (Platform Abstraction Layer)               ║
║  • I2C, SPI, GPIO, UART, ADC, PWM, I2S                          ║
║  • ⚠️  ONLY layer that #includes ESP-IDF / Zephyr headers        ║
║  • Swap this layer to port entire project to another RTOS        ║
╠═══════════════════════════════════════════════════════════════════╣
║                    ESP-IDF / Zephyr (hidden)                     ║
║  • Never referenced above PAL — completely invisible             ║
╚═══════════════════════════════════════════════════════════════════╝
```

## Key Architectural Rules

1. **No SDK leakage** — No `#include "esp_*.h"` or `#include <driver/...>` above the PAL layer
2. **Dependency direction** — Upper layers depend on lower layers, never the reverse
3. **Interface contracts** — Each layer defines abstract interfaces; implementations are injected
4. **Single responsibility** — Each component driver handles exactly one peripheral
5. **Portability by design** — Replace PAL + OSAL to run on Zephyr, NuttX, or bare metal

## GoF Design Patterns (14 Applied Automatically)

### Creational Patterns

| Pattern | Location | Purpose |
|---------|----------|---------|
| **Singleton** | OSAL managers, global config | Thread-safe single instances via `getInstance()` |
| **Factory** | Component creation | Encapsulated construction of drivers from config |
| **Builder** | Telemetry frame assembly | Step-by-step construction of data packets |

### Structural Patterns

| Pattern | Location | Purpose |
|---------|----------|---------|
| **Adapter** | Receiver interfaces | Uniform lifecycle (init/read/write/deinit) for all drivers |
| **Facade** | Analytics API, system API | Simplified access to complex subsystems |
| **Composite** | UI widget groups, component trees | Hierarchical composition of elements |
| **Proxy** | Remote access, lazy loading | Transparent caching and access control |
| **Decorator** | Signal processing pipeline | Chainable transformations (filter → scale → clamp) |

### Behavioral Patterns

| Pattern | Location | Purpose |
|---------|----------|---------|
| **Observer** | Event bus, pub/sub system | Decoupled sensor → actuator → backend communication |
| **Strategy** | Sensor read algorithms | Pluggable measurement strategies per sensor type |
| **Command** | Parameter adjustment, OTA | Encapsulated operations with undo/redo capability |
| **Chain of Responsibility** | Alert rules, validation | Priority-based evaluation of thresholds |
| **Template Method** | Task loop functions | Fixed init→loop→cleanup structure, customisable steps |
| **Mediator** | DataFlow engine | Cross-MCU coordination without direct coupling |

## Board Architecture

### theSpeaker (ESP32)
- **Role:** Master
- **Components:**
  - ILI9341 TFT Display (SPI)
  - XPT2046 Touch Controller (SPI)
  - RGB LED (GPIO)
  - LDR Light Sensor (ADC)
  - MicroSD Card Slot (SPI)
  - Speaker Connector (8002A Amp) (DAC)
  - Bluetooth Speaker (BLUETOOTH)

### environment (ESP32-C3)
- **Role:** Slave
- **Components:**
  - ST7735S LCD Display (SPI)
  - Joystick Up (GPIO)
  - Joystick Down (GPIO)
  - Joystick Left (GPIO)
  - Joystick Right (GPIO)
  - Joystick Center/Press (GPIO)
  - Boot Button (GPIO)
  - AHT10 Temperature/Humidity Sensor (I2C)

### motor_n_mike (ESP32C3)
- **Role:** Slave
- **Components:**
  - INMP441 I2S MEMS Microphone (I2S)


## File Statistics

| Layer | Files |
|-------|-------|
| PAL (Platform Abstraction) | 72 |
| OSAL (OS Abstraction) | 36 |
| Middleware | 9 |
| Total C/C++ source files | 619 |

## Cross-MCU Communication

This project uses the **DataFlow Engine** to synchronize variables across 3 MCUs.
The engine ensures that sensor data from any board is consistently available to the backend
and frontend without manual protocol implementation.

Protocols used: SPI, GPIO, ADC, DAC, TCP, HTTP, BLUETOOTH, I2S, I2C, REST_API

---

*Architecture enforced by CircuitOS Studio code generation engine.*
