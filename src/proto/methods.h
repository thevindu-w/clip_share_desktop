/*
 * proto/methods.h - declarations of methods
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

#ifndef PROTO_METHODS_H_
#define PROTO_METHODS_H_

#include <clients/status_cb.h>
#include <utils/net_utils.h>

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2

// Version 1 methods
extern int get_text_v1(socket_t *socket, StatusCallback *callback);
extern int send_text_v1(socket_t *socket, StatusCallback *callback);
#if PROTOCOL_MIN <= 1
extern int get_files_v1(socket_t *socket, StatusCallback *callback);
extern int send_file_v1(socket_t *socket, StatusCallback *callback);
#endif
extern int get_image_v1(socket_t *socket, StatusCallback *callback);
extern int info_v1(socket_t *socket);

// Version 2 methods
#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
extern int get_files_v2(socket_t *socket, StatusCallback *callback);
extern int send_files_v2(socket_t *socket, StatusCallback *callback);
#endif

// Version 3 methods
#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
extern int get_files_v3(socket_t *socket, StatusCallback *callback);
extern int send_files_v3(socket_t *socket, StatusCallback *callback);
extern int get_copied_image_v3(socket_t *socket, StatusCallback *callback);
extern int get_screenshot_v3(socket_t *socket, uint16_t display, StatusCallback *callback);
#endif

#endif  // PROTO_METHODS_H_
