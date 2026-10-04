/**
 * @file http_comm_common.c
 * @brief HTTP Common Utilities Implementation

 */

#include "http_comm_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char* http_url_encode(const char *str)
{
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *encoded = malloc(len * 3 + 1);
    if (!encoded) return NULL;
    
    char *p = encoded;
    while (*str) {
        if ((*str >= 'a' && *str <= 'z') ||
            (*str >= 'A' && *str <= 'Z') ||
            (*str >= '0' && *str <= '9') ||
            *str == '-' || *str == '_' || *str == '.' || *str == '~') {
            *p++ = *str;
        } else {
            p += sprintf(p, "%%%02X", (unsigned char)*str);
        }
        str++;
    }
    *p = '\0';
    return encoded;
}

void http_build_query(char *buffer, size_t max_len, const http_param_t *params, size_t param_count)
{
    if (!buffer || !params || max_len == 0) return;
    
    buffer[0] = '\0';
    size_t pos = 0;
    
    for (size_t i = 0; i < param_count && pos < max_len - 1; i++) {
        char *encoded_value = http_url_encode(params[i].value);
        if (!encoded_value) continue;
        
        int written = snprintf(buffer + pos, max_len - pos, "%s%s=%s",
                              (i == 0) ? "" : "&",
                              params[i].name,
                              encoded_value);
        free(encoded_value);
        
        if (written > 0) pos += written;
    }
}

bool http_parse_json_string(const char *json, const char *key, char *value, size_t max_len)
{
    if (!json || !key || !value || max_len == 0) return false;
    
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char *start = strstr(json, search);
    if (!start) return false;
    
    start += strlen(search);
    while (*start == ' ' || *start == '\t') start++;
    
    if (*start == '"') {
        start++;
        const char *end = strchr(start, '"');
        if (!end) return false;
        
        size_t len = end - start;
        if (len >= max_len) len = max_len - 1;
        memcpy(value, start, len);
        value[len] = '\0';
        return true;
    }
    
    return false;
}

bool http_parse_json_number(const char *json, const char *key, float *value)
{
    if (!json || !key || !value) return false;
    
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char *start = strstr(json, search);
    if (!start) return false;
    
    start += strlen(search);
    while (*start == ' ' || *start == '\t') start++;
    
    *value = strtof(start, NULL);
    return true;
}