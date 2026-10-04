/**
 * HTTP Client for sending data to backend
 * Generated for motor_n_mike
 * 
 * This module sends dataflow variables to the backend server via HTTP POST.
 */
#ifndef DATAFLOW_HTTP_CLIENT_H
#define DATAFLOW_HTTP_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

// Backend configuration
#define BACKEND_HOST "127.0.0.1"
#define BACKEND_PORT 8000
#define BACKEND_ENDPOINT "/api/data"
#define SEND_INTERVAL_MS 1000

// Data flow variables (extern declarations)
extern float g_dataflow_motor_n_mike_inmp441_audio_samples;
extern int32_t g_dataflow_motor_n_mike_inmp441_audio_level;
extern int32_t g_dataflow_motor_n_mike_inmp441_peak_amplitude;
extern float g_dataflow_motor_n_mike_inmp441_rms_level;
extern uint8_t g_dataflow_motor_n_mike_inmp441_is_clipping;

/**
 * Initialize the HTTP client.
 * Call this after WiFi is connected.
 */
void dataflow_http_init(void);

/**
 * Send all dataflow variables to the backend.
 * Returns true on success, false on failure.
 */
bool dataflow_http_send(void);

/**
 * Set custom backend URL.
 * @param host Backend server hostname or IP
 * @param port Backend server port
 */
void dataflow_http_set_backend(const char* host, int port);

#endif // DATAFLOW_HTTP_CLIENT_H