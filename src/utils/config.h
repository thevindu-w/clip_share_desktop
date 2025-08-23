/*
 * utils/config.h - header for conf_parse.c
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

#ifndef UTILS_CONFIG_H_
#define UTILS_CONFIG_H_

#include <stdint.h>
#include <sys/types.h>

typedef struct _config {
    uint16_t app_port;
    uint16_t web_port;
    char *working_dir;
    uint32_t bind_addr;
    uint32_t max_text_length;
    int64_t max_file_size;
    uint16_t min_proto_version;
    uint16_t max_proto_version;
    int8_t auto_send_text;
#if defined(_WIN32) || defined(__APPLE__)
    int8_t tray_icon;
#endif
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
