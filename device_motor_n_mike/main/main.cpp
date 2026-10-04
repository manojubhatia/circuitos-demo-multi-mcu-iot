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

// Forward declaration
extern "C" void hooks_system_init(void);

// Transport layer
#include "dataflow_spi_slave.h"

// Sensor base class and implementations
#include "base/Sensor.hpp"
#include "sensors/INMP441.hpp"

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

// inmp441 on i2s on i2s
INMP441 inmp441_i2s_mems_microphone(I2S_NUM_0, 0, 1, 2, 0);

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

static void comm_task(void* arg) {
    (void)arg;
    LOG_INFO("Communication task started");
    
    // Initialize transport(s)
    if (dataflow_spi_slave_init() != ESP_OK) {
        LOG_ERROR("Failed to initialize SPI slave");
    }
    
    uint8_t rx_buffer[128];
    osal_tick_t last_wake = osal_get_tick_count();
    
    while (true) {
        // Check for incoming SPI data (slave mode)
        int recv_len = dataflow_spi_slave_receive(rx_buffer, sizeof(rx_buffer));
        if (recv_len > 0) {
            mw_msg_sensor_data_t sensor_msg;
            memset(&sensor_msg, 0, sizeof(sensor_msg));
            sensor_msg.sensor_id = rx_buffer[0];
            memcpy(&sensor_msg.value, &rx_buffer[1], sizeof(float));
            sensor_msg.timestamp_ms = osal_get_time_ms();
            
            mw_publish_with_header(g_sensor_pub, &sensor_msg);
        }
        
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
            
            // Read INMP441 I2S MEMS Microphone
            {
                mw_msg_sensor_data_t msg;
                memset(&msg, 0, sizeof(msg));
                msg.sensor_id = 0;
                inmp441_i2s_mems_microphone.read();
                msg.value = inmp441_i2s_mems_microphone.readAudioLevel();
                msg.timestamp_ms = osal_get_time_ms();
                mw_publish_with_header(g_sensor_pub, &msg);
                telemetry_set_float("motor_n_mike_inmp441_audio_samples", msg.value);
                LOG_DEBUG("INMP441 I2S MEMS Microphone readAudioLevel: %.2f", msg.value);
                msg.value = inmp441_i2s_mems_microphone.readAudioLevel();
                msg.timestamp_ms = osal_get_time_ms();
                mw_publish_with_header(g_sensor_pub, &msg);
                telemetry_set_float("motor_n_mike_inmp441_audio_level", msg.value);
                LOG_DEBUG("INMP441 I2S MEMS Microphone readAudioLevel: %.2f", msg.value);
                msg.value = inmp441_i2s_mems_microphone.readPeakAmplitude();
                msg.timestamp_ms = osal_get_time_ms();
                mw_publish_with_header(g_sensor_pub, &msg);
                telemetry_set_float("motor_n_mike_inmp441_peak_amplitude", msg.value);
                LOG_DEBUG("INMP441 I2S MEMS Microphone readPeakAmplitude: %.2f", msg.value);
                msg.value = inmp441_i2s_mems_microphone.readRmsLevel();
                msg.timestamp_ms = osal_get_time_ms();
                mw_publish_with_header(g_sensor_pub, &msg);
                telemetry_set_float("motor_n_mike_inmp441_rms_level", msg.value);
                LOG_DEBUG("INMP441 I2S MEMS Microphone readRmsLevel: %.2f", msg.value);
                msg.value = inmp441_i2s_mems_microphone.readIsClipping();
                msg.timestamp_ms = osal_get_time_ms();
                mw_publish_with_header(g_sensor_pub, &msg);
                telemetry_set_float("motor_n_mike_inmp441_is_clipping", msg.value);
                LOG_DEBUG("INMP441 I2S MEMS Microphone readIsClipping: %.2f", msg.value);
            }
        }
        
        // Send sensor data to remote MCU via transport
        
        osal_task_delay_until(&last_wake, 1000);
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
            pal_mdns_init("motor_n_mike");
            pal_mdns_register_service("_http", "motor_n_mike", 80);
            pal_mdns_register_service("_ota", "motor_n_mike", 80);
            pal_mdns_register_service("_circuitos", "motor_n_mike", 80);
            LOG_INFO("mDNS services registered");
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
        

        // Send telemetry data to backend (paused while cause-action is streaming)
        if (!cause_action_is_streaming()) {
            telemetry_send_now();
        }

        
        
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
    LOG_INFO("Initializing INMP441 I2S MEMS Microphone...");
    if (inmp441_i2s_mems_microphone.init() != ESP_OK) {
        LOG_ERROR("Failed to initialize INMP441 I2S MEMS Microphone");
        CALL_ERROR_HOOK(-1, "Component init failed: INMP441 I2S MEMS Microphone");
    }
    else {
        esp_err_t rc = inmp441_i2s_mems_microphone.startRecording();
        if (rc == ESP_OK) {
            LOG_INFO("INMP441 I2S MEMS Microphone: I2S recording enabled");
        } else {
            LOG_ERROR("INMP441 I2S MEMS Microphone: startRecording() failed: %s", esp_err_to_name(rc));
        }
    }
    
    LOG_INFO("All components initialized");
    

    // ========================================================================
    // Initialize Commissioning (WiFi provisioning state machine)
    // ========================================================================
    LOG_INFO("Initializing commissioning...");
    commissioning_init();

    // Post-init hook - user code can now access initialized components
    CALL_HOOK_VOID(on_post_init);
    

    
    
    // ========================================================================
    // Create Application Tasks using OSAL
    // ========================================================================
    LOG_INFO("Creating application tasks...");
    
    // Communication task - handles inter-MCU protocol (Core 0: network)
    osal_task_config_t comm_cfg = {
        .name = "comm_task",
        .func = comm_task,
        .arg = NULL,
        .stack_size = 4096,
        .priority = OSAL_PRIORITY_HIGH,
        .pinned_to_core = false,
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
        .pinned_to_core = false,
        .core_id = 0,
    };
    if (osal_task_create(&sensor_cfg, &g_sensor_task_handle) != OSAL_OK) {
        LOG_ERROR("Failed to create sensor task");
    }

    // Main application task (Core 1: display + commissioning)
    osal_task_config_t main_cfg = {
        .name = "main_task",
        .func = main_task,
        .arg = NULL,
        .stack_size = 8192,
        .priority = OSAL_PRIORITY_NORMAL,
        .pinned_to_core = false,
        .core_id = 0,
    };
    if (osal_task_create(&main_cfg, &g_main_task_handle) != OSAL_OK) {
        LOG_ERROR("Failed to create main task");
    }
    
    LOG_INFO("All tasks created successfully");
    
}