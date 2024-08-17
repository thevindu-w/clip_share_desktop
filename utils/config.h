#ifndef UTILS_CONFIG_H_
#define UTILS_CONFIG_H_

#include <stdint.h>
#include <sys/types.h>

typedef struct _config {
    unsigned short app_port;
    uint32_t max_text_length;
    int64_t max_file_size;
    unsigned short min_proto_version;
    unsigned short max_proto_version;
} config;

/*
 * Parse the config file given by the file_name.
 * Sets the default configuration on error.
 */
extern void parse_conf(config *cfg, const char *file_name);

/*
 * Clears all the memory allocated in the array type or list2* type elements in the configuration.
 * This does not clear the memory block pointed by the config *conf.
 */
extern void clear_config(config *conf);

#endif  // UTILS_CONFIG_H_
