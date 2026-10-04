/**
 * @file middleware_types.h
 * @brief Pub/Sub Middleware - Type Definitions

 * 
 * Defines message types, topics, and QoS settings for the middleware.
 */

#ifndef MIDDLEWARE_TYPES_H
#define MIDDLEWARE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MW_MAX_TOPIC_NAME_LEN    48
#define MW_MAX_TOPICS            14
#define MW_MAX_SUBSCRIBERS       4
#define MW_MAX_MESSAGE_SIZE      128
#define MW_HISTORY_DEPTH         5

/* ============================================================================
 * QoS Settings
 * ============================================================================ */

/**
 * @brief Quality of Service profile
 */
typedef enum {
    MW_QOS_BEST_EFFORT = 0,     /**< Fire and forget, may drop messages */
    MW_QOS_RELIABLE,            /**< Guaranteed delivery with retry */
    MW_QOS_TRANSIENT_LOCAL      /**< Store last N messages for late joiners */
} mw_qos_t;

/**
 * @brief Message reliability
 */
typedef enum {
    MW_RELIABILITY_BEST_EFFORT = 0,
    MW_RELIABILITY_RELIABLE
} mw_reliability_t;

/**
 * @brief Message durability
 */
typedef enum {
    MW_DURABILITY_VOLATILE = 0, /**< No storage, only live messages */
    MW_DURABILITY_TRANSIENT,    /**< Store for late-joining subscribers */
    MW_DURABILITY_PERSISTENT    /**< Store to persistent storage */
} mw_durability_t;

/* ============================================================================
 * Base Message Types (ROS2-style)
 * ============================================================================ */

/**
 * @brief Standard message header (like ROS2 std_msgs/Header)
 */
typedef struct {
    uint32_t seq;               /**< Sequence number */
    uint64_t timestamp_us;      /**< Timestamp in microseconds */
    char frame_id[32];          /**< Frame of reference */
} mw_header_t;

/**
 * @brief Boolean message
 */
typedef struct {
    bool data;
} mw_msg_bool_t;

/**
 * @brief Integer message
 */
typedef struct {
    int32_t data;
} mw_msg_int32_t;

/**
 * @brief Unsigned integer message
 */
typedef struct {
    uint32_t data;
} mw_msg_uint32_t;

/**
 * @brief Float message
 */
typedef struct {
    float data;
} mw_msg_float_t;

/**
 * @brief Double message
 */
typedef struct {
    double data;
} mw_msg_double_t;

/**
 * @brief String message
 */
typedef struct {
    char data[128];
} mw_msg_string_t;

/**
 * @brief Byte array message
 */
typedef struct {
    uint8_t data[MW_MAX_MESSAGE_SIZE];
    size_t length;
} mw_msg_bytes_t;

/* ============================================================================
 * Sensor Message Types
 * ============================================================================ */

/**
 * @brief Temperature message
 */
typedef struct {
    mw_header_t header;
    float temperature_c;
    float variance;             /**< Measurement variance */
} mw_msg_temperature_t;

/**
 * @brief Humidity message
 */
typedef struct {
    mw_header_t header;
    float relative_humidity;    /**< 0-100% */
    float variance;
} mw_msg_humidity_t;

/**
 * @brief Pressure message
 */
typedef struct {
    mw_header_t header;
    float pressure_pa;
    float variance;
} mw_msg_pressure_t;

/**
 * @brief IMU message (accelerometer + gyroscope)
 */
typedef struct {
    mw_header_t header;
    struct {
        float x, y, z;
    } linear_acceleration;      /**< m/s^2 */
    struct {
        float x, y, z;
    } angular_velocity;         /**< rad/s */
    struct {
        float x, y, z, w;
    } orientation;              /**< Quaternion */
    float covariance[9];        /**< Row-major 3x3 */
} mw_msg_imu_t;

/**
 * @brief GPS/GNSS message
 */
typedef struct {
    mw_header_t header;
    double latitude;            /**< Degrees */
    double longitude;           /**< Degrees */
    double altitude;            /**< Meters above ellipsoid */
    float horizontal_accuracy;  /**< Meters */
    float vertical_accuracy;    /**< Meters */
    uint8_t fix_type;           /**< 0=none, 1=2D, 2=3D, 3=DGPS, 4=RTK */
    uint8_t num_satellites;
} mw_msg_gps_t;

/**
 * @brief Distance/Range message
 */
typedef struct {
    mw_header_t header;
    float range_m;              /**< Distance in meters */
    float min_range;            /**< Sensor minimum range */
    float max_range;            /**< Sensor maximum range */
    float field_of_view;        /**< Radians */
} mw_msg_range_t;

/**
 * @brief Battery status message
 */
typedef struct {
    mw_header_t header;
    float voltage;              /**< Volts */
    float current;              /**< Amps (negative = discharging) */
    float percentage;           /**< 0-100% */
    bool charging;
} mw_msg_battery_t;

/* ============================================================================
 * Actuator Message Types
 * ============================================================================ */

/**
 * @brief PWM command message
 */
typedef struct {
    uint8_t channel;
    uint16_t duty_cycle;        /**< 0-65535 */
    uint32_t frequency_hz;
} mw_msg_pwm_t;

/**
 * @brief Motor command message
 */
typedef struct {
    mw_header_t header;
    float velocity;             /**< Target velocity (rad/s or m/s) */
    float torque;               /**< Target torque (Nm) */
    float position;             /**< Target position (rad or m) */
    uint8_t mode;               /**< 0=velocity, 1=torque, 2=position */
} mw_msg_motor_cmd_t;

/**
 * @brief Servo command message
 */
typedef struct {
    uint8_t channel;
    float angle_deg;            /**< Target angle in degrees */
    float speed;                /**< Movement speed (deg/s), 0 = max */
} mw_msg_servo_cmd_t;

/**
 * @brief RGB LED command message
 */
typedef struct {
    uint8_t r, g, b;
    uint8_t brightness;         /**< 0-255 */
    uint16_t transition_ms;     /**< Fade duration */
} mw_msg_rgb_t;

/* ============================================================================
 * Display Message Types
 * ============================================================================ */

/**
 * @brief Text display message
 */
typedef struct {
    char text[128];
    uint8_t x, y;               /**< Position */
    uint8_t font_size;
    uint16_t color;             /**< RGB565 */
} mw_msg_display_text_t;

/**
 * @brief Display clear message
 */
typedef struct {
    uint16_t color;             /**< Fill color RGB565 */
} mw_msg_display_clear_t;

/* ============================================================================
 * Topic Handle
 * ============================================================================ */

/**
 * @brief Opaque topic handle
 */
typedef struct mw_topic_s* mw_topic_handle_t;

/**
 * @brief Opaque publisher handle
 */
typedef struct mw_publisher_s* mw_publisher_handle_t;

/**
 * @brief Opaque subscriber handle
 */
typedef struct mw_subscriber_s* mw_subscriber_handle_t;

/**
 * @brief Subscriber callback function type
 * @param topic Topic name
 * @param msg Pointer to message data
 * @param msg_size Size of message
 * @param user_data User-provided context
 */
typedef void (*mw_subscriber_callback_t)(const char* topic, const void* msg, size_t msg_size, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* MIDDLEWARE_TYPES_H */