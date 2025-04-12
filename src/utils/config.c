/*
 * utils/conf_parse.c - parse config file for application
 * Copyright (C) 2024-2025 H. Thevindu J. Wijesekera
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/config.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#define LINE_MAX_LEN 2047

/*
 * Trims all charactors in the range \x01 to \x20 inclusive from both ends of
 * the string in-place.
 * The string must point to a valid and null-terminated string.
 * This does not re-allocate memory to shrink.
 */
static inline void trim(char *str) {
    const char *ptr = str;
    while (0 < *ptr && *ptr <= ' ') {
        ptr++;
    }
    char *p1 = str;
    while (*ptr) {
        *p1 = *ptr;
        p1++;
        ptr++;
    }
    *p1 = 0;
    if (*str == 0) return;
    p1--;
    while (0 < *p1 && *p1 <= ' ') {
        *p1 = 0;
        p1--;
    }
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned 64-bit long integer
 * Sets the value pointed by conf_ptr to the unsigned 64-bit value given as a string in str if that is a valid value
 * between 1 and 2^61-1 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_int64(const char *str, int64_t *conf_ptr) {
    char *end_ptr;
    int64_t value = (int64_t)strtoull(str, &end_ptr, 10);
    switch (*end_ptr) {
        case '\0':
            break;
        case 'k':
        case 'K': {
            if (value > 2305843009213693LL) error_exit("Error: config value too large");
            value *= 1000;
            break;
        }
        case 'm':
        case 'M': {
            if (value > 2305843009213LL) error_exit("Error: config value too large");
            value *= 1000000L;
            break;
        }
        case 'g':
        case 'G': {
            if (value > 2305843009LL) error_exit("Error: config value too large");
            value *= 1000000000L;
            break;
        }
        case 't':
        case 'T': {
            if (value > 2305843L) error_exit("Error: config value too large");
            value *= 1000000000000LL;
            break;
        }
        default: {
            error_exit("Error: config value has invalid suffix");
        }
    }
    if (*end_ptr && *(end_ptr + 1)) error_exit("Error: config value has invalid suffix");
    if (value <= 0) error_exit("Error: invalid config value");
    *conf_ptr = value;
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned int
 * Sets the value pointed by conf_ptr to the unsigned int given as a string in str if that is a valid value between 1
 * and 2^32-2 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_uint32(const char *str, uint32_t *conf_ptr) {
    char *end_ptr;
    int64_t value = (int64_t)strtoull(str, &end_ptr, 10);
    if (value < 0 || value > 4294967294LL) error_exit("Error: config value not in range 0-4294967294");
    switch (*end_ptr) {
        case '\0':
            end_ptr--;
            break;
        case 'k':
        case 'K': {
            if (value > 4294967L) error_exit("Error: config value too large");
            value *= 1000;
            break;
        }
        case 'm':
        case 'M': {
            if (value > 4294) error_exit("Error: config value too large");
            value *= 1000000L;
            break;
        }
        case 'g':
        case 'G': {
            if (value > 4) error_exit("Error: config value too large");
            value *= 1000000000L;
            break;
        }
        default:
            error_exit("Error: config value has invalid suffix");
    }
    if (*end_ptr && *(end_ptr + 1)) error_exit("Error: config value has invalid suffix");
    if (value <= 0 || 4294967295LL <= value) error_exit("Error: invalid config value");
    *conf_ptr = (uint32_t)value;
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned short
 * Sets the value pointed by conf_ptr to the unsigned short given as a string in str if that is a valid value between 1
 * and 65535 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_uint16(const char *str, uint16_t *conf_ptr) {
    char *end_ptr;
    long value = strtol(str, &end_ptr, 10);
    if (value < 0 || value > 65535L) error_exit("Error: config value not in range 0-65535");
    if (*end_ptr) error_exit("Error: invalid config value");
    *conf_ptr = (uint16_t)value;
}

/*
 * Parse a single line in the config file and update the config if the line
 * contained a valid configuration.
 */
static void parse_line(char *line, config *cfg) {
    char *eq = strchr(line, '=');
    if (!eq) {
        return;
    }
    *eq = 0;
    char *key = line;
    char *value = eq + 1;

    trim(key);
    trim(value);

    if (key[0] == '#') return;

    const size_t key_len = strnlen(key, LINE_MAX_LEN);
    if (key_len <= 0 || key_len >= LINE_MAX_LEN) error_exit("Error: invalid config key");

    const size_t value_len = strnlen(value, LINE_MAX_LEN);
    if (value_len <= 0 || value_len >= LINE_MAX_LEN) error_exit("Error: invalid config value");

    if (!strcmp("app_port", key)) {
        set_uint16(value, &(cfg->app_port));
    } else if (!strcmp("web_port", key)) {
        set_uint16(value, &(cfg->web_port));
    } else if (!strcmp("working_dir", key)) {
        if (cfg->working_dir) free(cfg->working_dir);
        cfg->working_dir = strdup(value);
    } else if (!strcmp("max_text_length", key)) {
        set_uint32(value, &(cfg->max_text_length));
    } else if (!strcmp("max_file_size", key)) {
        set_int64(value, &(cfg->max_file_size));
    } else if (!strcmp("min_proto_version", key)) {
        set_uint16(value, &(cfg->min_proto_version));
    } else if (!strcmp("max_proto_version", key)) {
        set_uint16(value, &(cfg->max_proto_version));
#ifdef DEBUG_MODE
    } else {
        printf("Unknown key \"%s\"\n", key);
#endif
    }
}

void parse_conf(config *cfg, const char *file_name) {
    cfg->app_port = 0;
    cfg->web_port = 0;
    cfg->working_dir = NULL;
    cfg->max_text_length = 0;
    cfg->max_file_size = 0;
    cfg->min_proto_version = 0;
    cfg->max_proto_version = 0;

    if (!file_name) {
#ifdef DEBUG_MODE
        printf("File name is null\n");
#endif
        puts("Using default configurations");
        return;
    }

    FILE *f = fopen(file_name, "r");
    if (!f) {
        puts("Using default configurations");
        return;
    }

    char line[LINE_MAX_LEN + 1];
    while (fgets(line, LINE_MAX_LEN, f)) {
        line[LINE_MAX_LEN] = 0;
        parse_line(line, cfg);
    }
    fclose(f);

    return;
}

void clear_config(config *cfg) {
    if (cfg->working_dir) {
        free(cfg->working_dir);
        cfg->working_dir = NULL;
    }
}
