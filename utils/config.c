#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/config.h>
#include <utils/net_utils.h>

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
 * between 1 and 2^32-1 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_int64(const char *str, int64_t *conf_ptr) {
    char *end_ptr;
    int64_t value = (ssize_t)strtoull(str, &end_ptr, 10);
    switch (*end_ptr) {
        case '\0':
            break;
        case 'k':
        case 'K': {
            value *= 1000;
            break;
        }
        case 'm':
        case 'M': {
            value *= 1000000;
            break;
        }
        case 'g':
        case 'G': {
            value *= 1000000000;
            break;
        }
        case 't':
        case 'T': {
            value *= 1000000000000L;
            break;
        }
        default:
            return;
    }
    if (*end_ptr && *(end_ptr + 1)) return;
    if (0 < value) {
        *conf_ptr = value;
    }
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned int
 * Sets the value pointed by conf_ptr to the unsigned int given as a string in str if that is a valid value between 1
 * and 2^32-2 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_uint32(const char *str, uint32_t *conf_ptr) {
    char *end_ptr;
    long long value = strtoll(str, &end_ptr, 10);
    switch (*end_ptr) {
        case '\0':
            break;
        case 'k':
        case 'K': {
            value *= 1000;
            break;
        }
        case 'm':
        case 'M': {
            value *= 1000000;
            break;
        }
        case 'g':
        case 'G': {
            value *= 1000000000;
            break;
        }
        default:
            return;
    }
    if (*end_ptr && *(end_ptr + 1)) return;
    if (0 < value && value <= 4294967295L) {
        *conf_ptr = (uint32_t)value;
    }
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned short
 * Sets the value pointed by conf_ptr to the unsigned short given as a string in str if that is a valid value between 1
 * and 65535 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_ushort(const char *str, unsigned short *conf_ptr) {
    char *end_ptr;
    long value = strtol(str, &end_ptr, 10);
    if (*end_ptr) return;
    if (0 < value && value < 65536) {
        *conf_ptr = (unsigned short)value;
    }
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
    if (key_len <= 0 || key_len >= LINE_MAX_LEN) return;

    const size_t value_len = strnlen(value, LINE_MAX_LEN);
    if (value_len <= 0 || value_len >= LINE_MAX_LEN) return;

    if (!strcmp("app_port", key)) {
        set_ushort(value, &(cfg->app_port));
    } else if (!strcmp("max_text_length", key)) {
        set_uint32(value, &(cfg->max_text_length));
    } else if (!strcmp("max_file_size", key)) {
        set_int64(value, &(cfg->max_file_size));
    } else if (!strcmp("min_proto_version", key)) {
        set_ushort(value, &(cfg->min_proto_version));
    } else if (!strcmp("max_proto_version", key)) {
        set_ushort(value, &(cfg->max_proto_version));
#ifdef DEBUG_MODE
    } else {
        printf("Unknown key \"%s\"\n", key);
#endif
    }
}

void parse_conf(config *cfg, const char *file_name) {
    cfg->app_port = 0;
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
    // TODO (thevindu-w): implement for heap allocated config items
}
