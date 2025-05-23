/*
 * globals.h - header containing global variables
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

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <microhttpd.h>
#include <utils/config.h>

#define CONFIG_FILE "clipshare-desktop.conf"

extern config configuration;
extern char *error_log_file;
extern char *cwd;
extern size_t cwd_len;

extern struct MHD_Daemon *http_daemon;

#endif  // GLOBALS_H_
