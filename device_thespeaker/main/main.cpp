/**
 * @file main.cpp
 * @brief Main application entry point with OSAL tasks and Middleware pub/sub
 * @project theSpeaker (3 boards)

 * 
 * Architecture:
 *   - OSAL: OS Abstraction Layer (supports FreeRTOS, Zephyr, Linux, bare-metal)
 *   - Middleware: ROS2-style pub/sub for decoupled component communication
 *   - Tasks: Separate tasks for communication, sensors, actuators, main logic
 *   - Hooks: User-extensible hook system for custom logic
 * 
 * Section Types:
 *   - REGENERATABLE: Auto-generated, updated on regeneration
 *   - PROTECTED: User code, never overwritten
 *   - AI_EDITABLE: AI agents can modify via surgical edits
 */

// ════════════════════════════════════════════════════════════════════════════
// SECTION: INCLUDES (regeneratable)
// ════════════════════════════════════════════════════════════════════════════

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "osal/osal.h"
#include "osal/osal_task.h"
#include "osal/osal_log.h"
#include "osal/osal.h"
#include "pal/pal_storage.h"

// OSAL - OS Abstraction Layer
#include "osal/osal.h"

// Middleware - Pub/Sub message passing
#include "middleware/middleware.h"

// Hooks - User extension points
#include "hooks/lifecycle_hooks.h"
#include "hooks/component_hooks.h"
#include "display_ui.h"
#include "display_ui_dataflow.h"

// Forward declaration
extern "C" void hooks_system_init(void);

// Transport layer
#include "dataflow_wifi.h"
#if __has_include("dataflow_protocol_defs.h")
#include "dataflow_protocol_defs.h"
#endif
#if __has_include("dataflow_stream_registry.h")
#include "dataflow_stream_registry.h"
#endif

// Input variable callback — self-contained implementation
typedef void (*dataflow_input_cb_t)(const char* var_id, const void* value, size_t size);
static dataflow_input_cb_t s_input_var_callback = NULL;

// Input variable state (set by on_input_variable_update callback)
static volatile bool playing = false;
static volatile int32_t volume = 0;
static volatile float audio_source = 0.0f;

// Sensor base class and implementations
#include "base/Sensor.hpp"
#include "sensors/LDRLight.hpp"

// Communication components
#include "communication/BuiltinSdcard.hpp"

// Input components (touch controllers, encoders, etc.)
#include "input/BuiltinTouch.hpp"

// Actuator base class and implementations
#include "base/Actuator.hpp"
#include "actuators/RGBLed.hpp"
#include "actuators/Buzzer.hpp"
#include "actuators/PAM8403.hpp"

// Display base class and implementations
#include "base/Display.hpp"
#include "displays/BuiltinDisplay.hpp"

// Provisioning & WiFi
#include "ap_provisioning.h"
#include "commissioning_manager.h"
#include "commissioning_state.h"
#include "nvs_handler.h"
#include "wifi_station.h"
#include "pal_mdns.h"

// Telemetry (sends sensor data to backend)
#include "telemetry_sender.h"
#include "cause_action_probe.h"

#include "cmd_channel.h"

#include "nvs_flash.h"
#include "esp_err.h"

// ════════════════════════════════════════════════════════════════════════════
// END SECTION: INCLUDES
// ════════════════════════════════════════════════════════════════════════════

OSAL_LOG_MODULE_REGISTER(theSpeaker_(3_boards));

static const char* TAG __attribute__((unused)) = "theSpeaker_(3_boards)";
#define LOG_INFO(fmt, ...)  OSAL_LOGI(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) OSAL_LOGE(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  OSAL_LOGW(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) OSAL_LOGD(fmt, ##__VA_ARGS__)

using namespace embedded;

/* ============================================================================
 * Message Types for Middleware Pub/Sub
 * ============================================================================ */

// Generic sensor data message
typedef struct {
    mw_header_t header;
    uint8_t sensor_id;
    float value;
    uint32_t timestamp_ms;
} mw_msg_sensor_data_t;

// Command message (for inter-MCU communication)
typedef struct {
    mw_header_t header;
    uint8_t cmd_id;
    uint8_t data[16];
    uint8_t data_len;
} mw_msg_command_t;

/* ============================================================================
 * Task Handles and Configuration
 * ============================================================================ */

static osal_task_handle_t g_comm_task_handle = NULL;
static osal_task_handle_t g_sensor_task_handle = NULL;
static osal_task_handle_t g_actuator_task_handle = NULL;
static osal_task_handle_t g_main_task_handle = NULL;

// Mutex for shared data protection
static osal_mutex_handle_t g_data_mutex = NULL;

// Publishers
static mw_publisher_handle_t g_sensor_pub = NULL;
static mw_publisher_handle_t g_cmd_pub = NULL;

// Subscribers
static mw_subscriber_handle_t g_cmd_sub = NULL;
static mw_subscriber_handle_t g_sensor_sub = NULL;

/* ============================================================================
 * Component Instances
 * ============================================================================ */

// builtin-display on spi on spi
BuiltinDisplay ili9341_tft_display(15, 2, -1, 13, 14, 21);
// builtin-touch on spi on spi
BuiltinTouch xpt2046_touch_controller(33);
// builtin-rgb-led on gpio on gpio
RGBLed rgb_led(4, 16, 17, true);
// builtin-ldr on adc on adc
LDRLight ldr_light_sensor(4);
// builtin-sdcard on spi on spi
BuiltinSdcard microsd_card_slot(5, 4, 5);
// builtin-speaker on dac on dac
Buzzer speaker_connector_8002a_amp(26);
// bluetooth-speaker on bluetooth on bluetooth
PAM8403 bluetooth_speaker(4, 4);

/* ============================================================================
 * Middleware Callbacks
 * ============================================================================ */

/**
 * @brief Called when a command message is received
 */
static void on_command_received(const char* topic, const void* msg, size_t msg_size, void* user_data) {
    (void)user_data;
    const mw_msg_command_t* cmd = (const mw_msg_command_t*)msg;
    
    LOG_INFO("Command received on %s: cmd_id=%d, seq=%lu", topic, cmd->cmd_id, (unsigned long)cmd->header.seq);
    
    // Process command and actuate
    switch (cmd->cmd_id) {
        case 0:
            // Command for RGB LED
            // rgb_led.setValue(cmd->data[0]);
            break;
        case 1:
            // Command for Speaker Connector (8002A Amp)
            // speaker_connector_8002a_amp.setValue(cmd->data[0]);
            break;
        case 2:
            // Command for Bluetooth Speaker
            // bluetooth_speaker.setValue(cmd->data[0]);
            break;
        default:
            LOG_DEBUG("Unknown command: %d", cmd->cmd_id);
            break;
    }
}

/**
 * @brief Callback for incoming dataflow packets that set input variables on this board.
 * Called by the exchange task when a remote board or the backend sends a value update.
 */
static void on_input_variable_update(const char* var_id, const void* value, size_t size) {
    if (!var_id || !value) return;
    
    // Strip "board_name::" prefix for matching
    const char* sep = strstr(var_id, "::");
    const char* lookup = sep ? (sep + 2) : var_id;
    
    if (strcmp(lookup, "playing") == 0 || strcmp(var_id, "playing") == 0) {
        if (size >= sizeof(float)) {
            float fval; memcpy(&fval, value, sizeof(float));
            playing = (fval != 0.0f);
        } else if (size >= 1) {
            playing = (*(const uint8_t*)value) != 0;
        }
        LOG_INFO("Input: playing = %s", playing ? "true" : "false");
        return;
    }
    if (strcmp(lookup, "volume") == 0 || strcmp(var_id, "volume") == 0) {
        if (size >= sizeof(int32_t)) {
            int32_t _tmp; memcpy(&_tmp, value, sizeof(int32_t));
            volume = _tmp;
        } else if (size >= sizeof(float)) {
            float fval; memcpy(&fval, value, sizeof(float));
            volume = (int32_t)fval;
        }
        LOG_INFO("Input: volume = %d", (int)volume);
        return;
    }
    if (strcmp(lookup, "audio_source") == 0 || strcmp(var_id, "audio_source") == 0) {
        if (size <= sizeof(audio_source)) {
            memcpy((void*)&audio_source, value, size);
        }
        return;
    }
}

/**
 * @brief Handles commands received from the backend via the command channel.
 * Called from cmd_channel (HTTP POST /cmd, UART, or BLE).
 *
 * For input variables (bidirectional IO), the device applies the value and sends
 * an ACK frame back to the backend via telemetry so the frontend can confirm
 * the state change.
 */
static void on_backend_command(const char *type, const char *variable_id, int value) {
    if (!type || !variable_id) return;

    /* Stream control: backend dispatches begin_stream/stop_stream when frontend
       navigates to/from a tab that subscribes to data.
       The 'value' field carries the frame_id when available, otherwise we
       look up the frame by variable_id from frame_descriptors.h. */
    if (strcmp(type, "begin_stream") == 0 || strcmp(type, "start_stream") == 0) {
        uint16_t frame_id = (uint16_t)value;
#if DATAFLOW_WIFI_HAS_FRAME_DESCS
        if (frame_id == 0) {
            /* Try to find frame_id by matching variable_id against field names */
            for (int i = 0; i < DF_FRAME_COUNT && frame_id == 0; i++) {
                if (g_all_frame_descriptors[i]) {
                    for (uint16_t fi = 0; fi < g_all_frame_descriptors[i]->field_count; fi++) {
                        if (strstr(g_all_frame_descriptors[i]->fields[fi].name, variable_id) != NULL) {
                            frame_id = g_all_frame_descriptors[i]->frame_id;
                            break;
                        }
                    }
                }
            }
        }
#endif
        if (frame_id != 0) {
            stream_registry_start(frame_id);
            LOG_INFO("begin_stream: frame_id=0x%04X (%s)", (unsigned)frame_id, variable_id);
        } else {
            LOG_WARN("begin_stream: could not find frame_id for '%s'", variable_id);
        }
        return;
    }
    
    if (strcmp(type, "stop_stream") == 0) {
        uint16_t frame_id = (uint16_t)value;
#if DATAFLOW_WIFI_HAS_FRAME_DESCS
        if (frame_id == 0) {
            for (int i = 0; i < DF_FRAME_COUNT && frame_id == 0; i++) {
                if (g_all_frame_descriptors[i]) {
                    for (uint16_t fi = 0; fi < g_all_frame_descriptors[i]->field_count; fi++) {
                        if (strstr(g_all_frame_descriptors[i]->fields[fi].name, variable_id) != NULL) {
                            frame_id = g_all_frame_descriptors[i]->frame_id;
                            break;
                        }
                    }
                }
            }
        }
#endif
        if (frame_id != 0) {
            stream_registry_stop(frame_id);
            LOG_INFO("stop_stream: frame_id=0x%04X (%s)", (unsigned)frame_id, variable_id);
        } else {
            LOG_WARN("stop_stream: could not find frame_id for '%s'", variable_id);
        }
        return;
    }

    // Forward backend commands to display UI for visual feedback
    display_ui_dataflow_update(variable_id, &value, sizeof(value));

    if (strcmp(type, "set_output") == 0) {
        bool on = (value != 0);
        bool handled = false;
        (void)on;

        // Send ACK back to backend so frontend can confirm the state change
        telemetry_set_float(variable_id, (float)value);

        // Input variable bindings — apply value and send ACK
        if (s_input_var_callback) {
            // Strip board prefix for matching (e.g. "environment::streaming_allowed" -> "streaming_allowed")
            const char* lookup = variable_id;
            const char* sep = strstr(variable_id, "::");
            if (sep) lookup = sep + 2;

            if (strcmp(lookup, "playing") == 0 || strcmp(variable_id, "playing") == 0) {
                float fval = (float)value;
                s_input_var_callback(variable_id, &fval, sizeof(fval));
                LOG_INFO("Backend cmd -> input var '%s' = %d (applied)", variable_id, value);
                handled = true;
                // Send ACK back to backend so frontend can confirm the state change
                telemetry_set_float(variable_id, fval);
            }
            if (strcmp(lookup, "volume") == 0 || strcmp(variable_id, "volume") == 0) {
                float fval = (float)value;
                s_input_var_callback(variable_id, &fval, sizeof(fval));
                LOG_INFO("Backend cmd -> input var '%s' = %d (applied)", variable_id, value);
                handled = true;
                // Send ACK back to backend so frontend can confirm the state change
                telemetry_set_float(variable_id, fval);
            }
            if (strcmp(lookup, "audio_source") == 0 || strcmp(variable_id, "audio_source") == 0) {
                float fval = (float)value;
                s_input_var_callback(variable_id, &fval, sizeof(fval));
                LOG_INFO("Backend cmd -> input var '%s' = %d (applied)", variable_id, value);
                handled = true;
                // Send ACK back to backend so frontend can confirm the state change
                telemetry_set_float(variable_id, fval);
            }
        }

        if (!handled) {
            /* Extract channel number from variable_id (e.g. "relay_ch2" -> 2) */
            int channel = 0;
            const char *ch_ptr = strstr(variable_id, "_ch");
            if (ch_ptr) channel = atoi(ch_ptr + 3);
            (void)channel;

            LOG_INFO("Backend cmd -> RGB LED var=%s val=%d", variable_id, value);
            LOG_INFO("Backend cmd -> Speaker Connector (8002A Amp) var=%s val=%d", variable_id, value);
            LOG_INFO("Backend cmd -> Bluetooth Speaker var=%s val=%d", variable_id, value);
        }
        (void)handled;
    }
}

/**
 * @brief Called when sensor data is received (for receiver MCU)
 */
static void on_sensor_data_received(const char* topic, const void* msg, size_t msg_size, void* user_data) {
    (void)user_data;
    const mw_msg_sensor_data_t* sensor_msg = (const mw_msg_sensor_data_t*)msg;
    
    LOG_DEBUG("Sensor data on %s: id=%d, value=%.2f", topic, sensor_msg->sensor_id, sensor_msg->value);
    
    // Lock mutex for thread-safe data access
    osal::MutexGuard lock(g_data_mutex);
    
    // Process received sensor data
    // This is where you would update displays, log data, or forward to cloud
}

/* ============================================================================
 * Communication Task - Handles inter-MCU protocol
 * ============================================================================ */

/**
 * @brief Protocol-aware frame callback
 *
 * Handles parsed protocol frames (with df_wire_header_t already decoded).
 * Dispatches control frames (START_STREAM, STOP_STREAM) to the stream registry,
 * and data frames to the application.
 *
 * This handler is needed for ALL TCP-connected boards:
 *   - Servers receive protocol frames from clients (including START/STOP)
 *   - Clients receive protocol frames from servers (including ACKs, DATA)
 *   - Both sides can initiate or respond to stream control
 */
static void dataflow_protocol_frame_handler(uint16_t frame_id, uint8_t frame_type,
                                             uint16_t sequence,
                                             const uint8_t* payload, size_t payload_len) {
    (void)sequence;
    
    switch (frame_type) {
    case DF_FRAME_TYPE_START_STREAM:
        stream_registry_start(frame_id);
        dataflow_wifi_send_control(frame_id, DF_FRAME_TYPE_ACK);
        LOG_INFO("START_STREAM ACKed for frame 0x%04X", (unsigned)frame_id);
        break;
        
    case DF_FRAME_TYPE_STOP_STREAM:
        stream_registry_stop(frame_id);
        dataflow_wifi_send_control(frame_id, DF_FRAME_TYPE_ACK);
        LOG_INFO("STOP_STREAM ACKed for frame 0x%04X", (unsigned)frame_id);
        break;
        
    case DF_FRAME_TYPE_HEARTBEAT:
        LOG_DEBUG("Heartbeat received (frame 0x%04X)", (unsigned)frame_id);
        break;
        
    case DF_FRAME_TYPE_ACK:
        LOG_DEBUG("ACK received for frame 0x%04X", (unsigned)frame_id);
        break;
        
    case DF_FRAME_TYPE_NACK:
        LOG_WARN("NACK received for frame 0x%04X", (unsigned)frame_id);
        break;
        
    case DF_FRAME_TYPE_DATA:
    case DF_FRAME_TYPE_BEGIN:
    case DF_FRAME_TYPE_CONTINUE:
    case DF_FRAME_TYPE_END:
        /* Data-carrying frames: extract variable values from payload using
           frame_descriptors.h field layouts and dispatch to application */
        if (payload && payload_len > 0) {
#if defined(DATAFLOW_FRAME_DESCRIPTOR_H) || __has_include("dataflow_frame_descriptor.h")
            /* Unpack each field from the frame and dispatch individually */
            {
                const df_frame_descriptor_t* desc = df_get_frame_descriptor(frame_id);
                if (desc != NULL) {
                    for (uint16_t fi = 0; fi < desc->field_count; fi++) {
                        const df_field_descriptor_t* fld = &desc->fields[fi];
                        if (fld->offset + fld->size <= payload_len) {
                            display_ui_dataflow_update(fld->name, &payload[fld->offset], fld->size);
                            if (s_input_var_callback != NULL) {
                                s_input_var_callback(fld->name, &payload[fld->offset], fld->size);
                            }
                            if (fld->size == sizeof(float)) {
                                float _fv;
                                memcpy(&_fv, &payload[fld->offset], sizeof(float));
                                telemetry_set_float(fld->name, _fv);
                                LOG_DEBUG("Frame 0x%04X field '%s' = %.2f",
                                          (unsigned)frame_id, fld->name, _fv);
                            }
                        }
                    }
                } else {
                    LOG_WARN("No descriptor for frame 0x%04X", (unsigned)frame_id);
                }
            }
#else
            /* Forward packed payload to display for visualization */
            {
                char frame_name[32];
                snprintf(frame_name, sizeof(frame_name), "frame_0x%04X", (unsigned)frame_id);
                display_ui_dataflow_update(frame_name, payload, payload_len);
            }
            /* Forward to input variable callback for application-level handling */
            if (s_input_var_callback != NULL) {
                char frame_name[32];
                snprintf(frame_name, sizeof(frame_name), "frame_0x%04X", (unsigned)frame_id);
                s_input_var_callback(frame_name, payload, payload_len);
            }
#endif
            LOG_DEBUG("Data frame: id=0x%04X type=%s len=%zu",
                      (unsigned)frame_id, df_frame_type_name(frame_type), payload_len);
        }
        break;
        
    case DF_FRAME_TYPE_REQUEST:
        /* REQUEST frames — config dump: respond with BEGIN/CONTINUE/END */
        LOG_INFO("REQUEST received for frame 0x%04X (config dump, %zu bytes)",
                 (unsigned)frame_id, payload_len);
        /* No adjustable parameters on this device — send empty END */
        dataflow_wifi_send_control(frame_id, DF_FRAME_TYPE_END);
        break;
        
    default:
        LOG_WARN("Unknown frame type 0x%02X for frame 0x%04X",
                 (unsigned)frame_type, (unsigned)frame_id);
        break;
    }
}

static void dataflow_wifi_rx_handler(const uint8_t* data, size_t len) {
    // Legacy frame format: [var_id_len(1)] [var_id(N)] [float(4)]
    if (len < 6) return;
    uint8_t var_id_len = data[0];
    if (len < (size_t)(1 + var_id_len + sizeof(float))) return;
    
    char var_id[128];
    if (var_id_len >= sizeof(var_id)) return;
    memcpy(var_id, &data[1], var_id_len);
    var_id[var_id_len] = '\0';
    
    float value;
    memcpy(&value, &data[1 + var_id_len], sizeof(float));
    
    display_ui_dataflow_update(var_id, &value, sizeof(value));
    // Forward to input variable callback for application-level handling
    if (s_input_var_callback != NULL) {
        s_input_var_callback(var_id, &value, sizeof(value));
    }
    LOG_DEBUG("WiFi RX: %s = %.2f", var_id, value);
}

static void comm_task(void* arg) {
    (void)arg;
    LOG_INFO("Communication task started");
    
    // Initialize transport(s)
    // Wait for WiFi to connect before starting TCP transport
    LOG_INFO("Waiting for WiFi before TCP transport...");
    while (!wifi_station_is_connected()) {
        osal_task_delay_ms(500);
    }
    LOG_INFO("WiFi connected, starting TCP transport");
    dataflow_wifi_set_rx_callback(dataflow_wifi_rx_handler);
    /* Protocol-aware frame callback and stream registry initialization */
    stream_registry_init();
    dataflow_wifi_set_frame_callback(dataflow_protocol_frame_handler);
    if (dataflow_wifi_init() != ESP_OK) {
        LOG_ERROR("Failed to initialize WiFi TCP transport");
    }
    
    osal_tick_t last_wake = osal_get_tick_count();
    
    while (true) {
        
        // Process middleware messages
        mw_spin_once(0);
        
        // Periodic delay
        osal_task_delay_until(&last_wake, 10);
    }
}

/* ============================================================================
 * Sensor Task - Reads sensors and publishes data
 * ============================================================================ */

static void sensor_task(void* arg) {
    (void)arg;
    LOG_INFO("Sensor task started");
    
    
    osal_tick_t last_wake = osal_get_tick_count();
    
    while (true) {
        // Lock mutex for thread-safe sensor access
        {
            osal::MutexGuard lock(g_data_mutex);
            
            // Read LDR Light Sensor
            {
                mw_msg_sensor_data_t msg;
                memset(&msg, 0, sizeof(msg));
                msg.sensor_id = 0;
                msg.value = ldr_light_sensor.readRaw();
                msg.timestamp_ms = osal_get_time_ms();
                mw_publish_with_header(g_sensor_pub, &msg);
                telemetry_set_float("ldr_light_sensor", msg.value);
                display_ui_dataflow_update("theSpeaker::ldr_light_sensor", &msg.value, sizeof(msg.value));
                LOG_DEBUG("LDR Light Sensor: %.2f", msg.value);
            }
        }
        
        // Send sensor data to remote MCU via transport
        
        osal_task_delay_until(&last_wake, 1000);
    }
}

/* ============================================================================
 * Actuator Task - Subscribes to commands and controls actuators
 * ============================================================================ */

static void actuator_task(void* arg) {
    (void)arg;
    LOG_INFO("Actuator task started");
    
    osal_tick_t last_wake = osal_get_tick_count();
    
    while (true) {
        // Process middleware messages (callbacks will be invoked)
        mw_spin_once(50);
        
        // Update RGB LED
        rgb_led.update();
        // Update Speaker Connector (8002A Amp)
        speaker_connector_8002a_amp.update();
        // Update Bluetooth Speaker
        bluetooth_speaker.update();
        
        osal_task_delay_until(&last_wake, 50);
    }
}

/* ============================================================================
 * Main Task - High-level application logic
 * ============================================================================ */

static void main_task(void* arg) {
    (void)arg;
    LOG_INFO("Main application task started");
    
    // Initialize telemetry sender
    TelemetryConfig telemetry_cfg = DEFAULT_TELEMETRY_CONFIG;
    telemetry_init(&telemetry_cfg);
    // Override backend target from NVS if configured during provisioning
    char backend_ip[64] = {0};
    if (nvs_read_backend_ip(backend_ip, sizeof(backend_ip)) && strlen(backend_ip) > 0) {
        uint16_t backend_port = nvs_read_backend_port();
        telemetry_set_target(backend_ip, backend_port);
        LOG_INFO("Backend from NVS: %s:%d", backend_ip, (int)backend_port);
    }
    // Start cause-action probe on a dedicated task (non-blocking)
    {
        CauseActionConfig ca_cfg = DEFAULT_CAUSE_ACTION_CONFIG;
        ca_cfg.stream_interval_ms = telemetry_get_interval_ms();
        cause_action_start(&ca_cfg);
    }

    // Network services flag — initialized lazily when WiFi connects
    static bool s_net_services_started = false;

    // Try initial WiFi wait (up to 30s), driving commissioning state machine
    LOG_INFO("Waiting for WiFi connection before starting network services...");
    for (int attempt = 0; attempt < 60; attempt++) {
        commissioning_loop();
        if (wifi_station_is_connected()) break;
        osal_task_delay_ms(500);
    }

    osal_tick_t last_wake = osal_get_tick_count();
    uint32_t loop_count = 0;
    
    while (true) {
        loop_count++;

        // Lazy-init network services once WiFi connects
        if (!s_net_services_started && wifi_station_is_connected()) {
            s_net_services_started = true;
            pal_mdns_init("thespeaker");
            pal_mdns_register_service("_http", "thespeaker", 80);
            pal_mdns_register_service("_ota", "thespeaker", 80);
            pal_mdns_register_service("_circuitos", "thespeaker", 80);
            LOG_INFO("mDNS services registered");
            {
                cmd_channel_config_t cmd_cfg = CMD_CHANNEL_DEFAULT_CONFIG;
                cmd_channel_init(&cmd_cfg);
                cmd_channel_set_callback(on_backend_command);
                LOG_INFO("Command channel ready on port %d", cmd_cfg.http_port);
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // Hook: Loop Start
        // ════════════════════════════════════════════════════════════════════
        CALL_HOOK_VOID(on_loop_start);
        
        // Run commissioning state machine (WiFi provisioning)
        commissioning_loop();

        
        // Periodic status report
        if (loop_count % 10 == 0) {
            LOG_INFO("Main loop iteration %lu, free heap: %u bytes", 
                    (unsigned long)loop_count, (unsigned)osal_get_free_heap());
            
            // Middleware statistics
            LOG_DEBUG("MW stats: pub=%lu, deliver=%lu, drop=%lu",
                     (unsigned long)mw_get_publish_count(),
                     (unsigned long)mw_get_deliver_count(),
                     (unsigned long)mw_get_drop_count());
        }
        

        // ════════════════════════════════════════════════════════════════════
        // Poll Input Components (touch controllers, encoders, etc.)
        // ════════════════════════════════════════════════════════════════════
        xpt2046_touch_controller.poll();

        // Send telemetry data to backend (paused while cause-action is streaming)
        if (!cause_action_is_streaming()) {
            telemetry_send_now();
        }

        // Update system metrics on display via dataflow
        {
            static uint32_t _heap_total = 0;
            if (_heap_total == 0) _heap_total = osal_get_free_heap();
            float _ram_pct = 100.0f - ((float)osal_get_free_heap() * 100.0f / (float)_heap_total);
            if (_ram_pct < 0.0f) _ram_pct = 0.0f;
            display_ui_dataflow_update("sys::ram_usage_pct", &_ram_pct, sizeof(_ram_pct));
            
            float _task_cnt = (float)osal_get_task_count();
            display_ui_dataflow_update("sys::task_count", &_task_cnt, sizeof(_task_cnt));
            
            // CPU usage: 100% - idle%, measured from OSAL runtime stats
            {
                static uint32_t _prev_idle_us = 0;
                static uint32_t _prev_total_us = 0;
                uint32_t idle_us = osal_get_idle_runtime_us();
                uint32_t total_us = (uint32_t)(osal_get_time_ms() * 1000ULL & 0xFFFFFFFF);
                float _cpu_pct = 0.0f;
                uint32_t dt = total_us - _prev_total_us;
                if (dt > 0) {
                    uint32_t di = idle_us - _prev_idle_us;
                    _cpu_pct = 100.0f - ((float)di * 100.0f / (float)dt);
                    if (_cpu_pct < 0.0f) _cpu_pct = 0.0f;
                    if (_cpu_pct > 100.0f) _cpu_pct = 100.0f;
                }
                _prev_idle_us = idle_us;
                _prev_total_us = total_us;
                display_ui_dataflow_update("sys::cpu_usage_pct", &_cpu_pct, sizeof(_cpu_pct));
            }
            float _rssi = wifi_station_is_connected() ? (float)wifi_station_get_rssi() : -127.0f;
            display_ui_dataflow_update("sys::wifi_rssi", &_rssi, sizeof(_rssi));
        }
        // Update displays
        CALL_DISPLAY_PRE_HOOK(0);
        ili9341_tft_display.update();
        CALL_DISPLAY_POST_HOOK(0);
        display_ui_tick();
        
        
        // ════════════════════════════════════════════════════════════════════
        // Hook: Loop End
        // ════════════════════════════════════════════════════════════════════
        CALL_HOOK_VOID(on_loop_end);
        
        // Tick hook with elapsed time
        uint32_t tick_ms = osal_get_time_ms();
        CALL_HOOK_1(on_tick, tick_ms);
        
        osal_task_delay_until(&last_wake, 1000);
    }
}

/* ============================================================================
 * Application Entry Point
 * ============================================================================ */

extern "C" void app_main(void) {
    LOG_INFO("Starting theSpeaker (3 boards)");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    if (!nvs_handler_init()) {
        LOG_ERROR("NVS handler init failed");
    }
    
    // ========================================================================
    // Initialize OSAL
    // ========================================================================
    LOG_INFO("Initializing OSAL...");
    if (osal_init() != OSAL_OK) {
        LOG_ERROR("Failed to initialize OSAL");
        return;
    }
    
    // Create data protection mutex
    if (osal_mutex_create(&g_data_mutex) != OSAL_OK) {
        LOG_ERROR("Failed to create data mutex");
    }
    
    // ========================================================================
    // Initialize Middleware Pub/Sub
    // ========================================================================
    LOG_INFO("Initializing Middleware...");
    if (mw_init() != 0) {
        LOG_ERROR("Failed to initialize Middleware");
        return;
    }
    
    // Create publishers
    if (mw_create_publisher("sensors/data", sizeof(mw_msg_sensor_data_t), MW_QOS_RELIABLE, &g_sensor_pub) != 0) {
        LOG_ERROR("Failed to create sensor publisher");
    }
    
    if (mw_create_publisher("commands/out", sizeof(mw_msg_command_t), MW_QOS_RELIABLE, &g_cmd_pub) != 0) {
        LOG_ERROR("Failed to create command publisher");
    }
    
    // Create subscribers
    if (mw_create_subscriber("commands/in", on_command_received, NULL, MW_QOS_RELIABLE, &g_cmd_sub) != 0) {
        LOG_ERROR("Failed to create command subscriber");
    }
    
    if (mw_create_subscriber("sensors/remote", on_sensor_data_received, NULL, MW_QOS_RELIABLE, &g_sensor_sub) != 0) {
        LOG_ERROR("Failed to create sensor subscriber");
    }
    
    // ========================================================================
    // Initialize Hooks System
    // ========================================================================
    LOG_INFO("Initializing hooks system...");
    hooks_system_init();
    
    // Pre-init hook
    CALL_HOOK_VOID(on_pre_init);
    
    // ========================================================================
    // Initialize Components
    // ========================================================================
    LOG_INFO("Initializing ILI9341 TFT Display...");
    if (ili9341_tft_display.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize ILI9341 TFT Display");
        CALL_ERROR_HOOK(-1, "Component init failed: ILI9341 TFT Display");
    }
    LOG_INFO("Initializing XPT2046 Touch Controller...");
    if (xpt2046_touch_controller.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize XPT2046 Touch Controller");
        CALL_ERROR_HOOK(-1, "Component init failed: XPT2046 Touch Controller");
    }
    LOG_INFO("Initializing RGB LED...");
    if (rgb_led.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize RGB LED");
        CALL_ERROR_HOOK(-1, "Component init failed: RGB LED");
    }
    LOG_INFO("Initializing LDR Light Sensor...");
    if (ldr_light_sensor.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize LDR Light Sensor");
        CALL_ERROR_HOOK(-1, "Component init failed: LDR Light Sensor");
    }
    LOG_INFO("Initializing MicroSD Card Slot...");
    if (microsd_card_slot.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize MicroSD Card Slot");
        CALL_ERROR_HOOK(-1, "Component init failed: MicroSD Card Slot");
    }
    LOG_INFO("Initializing Speaker Connector (8002A Amp)...");
    if (speaker_connector_8002a_amp.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize Speaker Connector (8002A Amp)");
        CALL_ERROR_HOOK(-1, "Component init failed: Speaker Connector (8002A Amp)");
    }
    LOG_INFO("Initializing Bluetooth Speaker...");
    if (bluetooth_speaker.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize Bluetooth Speaker");
        CALL_ERROR_HOOK(-1, "Component init failed: Bluetooth Speaker");
    }
    
    LOG_INFO("All components initialized");
    

    // ========================================================================
    // Initialize Commissioning (WiFi provisioning state machine)
    // ========================================================================
    LOG_INFO("Initializing commissioning...");
    commissioning_init();

    // Register input variable callback (sets local vars from incoming dataflow packets)
    s_input_var_callback = on_input_variable_update;
    LOG_INFO("Input variable callback registered (3 binding(s))");

    // Post-init hook - user code can now access initialized components
    CALL_HOOK_VOID(on_post_init);
    
    // Raw display: bind driver and init UI (LVGL displays do this in BuiltinDisplay::init)
    display_ui_init();
    display_ui_dataflow_set_ready();

    
    
    // ========================================================================
    // Create Application Tasks using OSAL
    // ========================================================================
    LOG_INFO("Creating application tasks...");
    
    // Communication task - handles inter-MCU protocol (Core 0: network)
    osal_task_config_t comm_cfg = {
        .name = "comm_task",
        .func = comm_task,
        .arg = NULL,
        .stack_size = 2048,
        .priority = OSAL_PRIORITY_HIGH,
        .pinned_to_core = true,
        .core_id = 0,
    };
    if (osal_task_create(&comm_cfg, &g_comm_task_handle) != OSAL_OK) {
        LOG_ERROR("Failed to create comm task");
    }
    
    // Sensor task - reads sensors and publishes data (Core 1: I/O + display)
    osal_task_config_t sensor_cfg = {
        .name = "sensor_task",
        .func = sensor_task,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_PRIORITY_NORMAL,
        .pinned_to_core = true,
        .core_id = 1,
    };
    if (osal_task_create(&sensor_cfg, &g_sensor_task_handle) != OSAL_OK) {
        LOG_ERROR("Failed to create sensor task");
    }

    // Actuator task (Core 1)
    osal_task_config_t actuator_cfg = {
        .name = "actuator_task",
        .func = actuator_task,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_PRIORITY_ABOVE_NORMAL,
        .pinned_to_core = true,
        .core_id = 1,
    };
    if (osal_task_create(&actuator_cfg, &g_actuator_task_handle) != OSAL_OK) {
        LOG_ERROR("Failed to create actuator task");
    }

    // Main application task (Core 1: display + commissioning)
    osal_task_config_t main_cfg = {
        .name = "main_task",
        .func = main_task,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_PRIORITY_NORMAL,
        .pinned_to_core = true,
        .core_id = 1,
    };
    if (osal_task_create(&main_cfg, &g_main_task_handle) != OSAL_OK) {
        LOG_ERROR("Failed to create main task");
    }
    
    LOG_INFO("All tasks created successfully");
    
}