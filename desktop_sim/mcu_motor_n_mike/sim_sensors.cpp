#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log_sim.h"

static int _sim_init = 0;

float sim_get_sensor_value(const char* name) {
    if (!_sim_init) { srand((unsigned)time(NULL)); _sim_init = 1; }
    return 25.0f + 5.0f * sinf((float)clock() / CLOCKS_PER_SEC);
}

float sim_get_sensor_value_by_index(int idx) { (void)idx; return sim_get_sensor_value("default"); }

#ifdef __cplusplus
}
#endif