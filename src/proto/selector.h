/*
 * proto/selector.h - header for declaring protocol handler
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

#ifndef PROTO_SELECTOR_H_
#define PROTO_SELECTOR_H_

#include <clients/status_cb.h>
#include <stdint.h>
#include <utils/net_utils.h>

// methods
#define METHOD_GET_TEXT 1
#define METHOD_SEND_TEXT 2
#define METHOD_GET_FILE 3
#define METHOD_SEND_FILE 4
#define METHOD_GET_IMAGE 5
#define METHOD_GET_COPIED_IMAGE 6
#define METHOD_GET_SCREENSHOT 7
#define METHOD_INFO 125

typedef union {
    uint16_t display;
    int8_t is_auto_send;
} MethodArgs;

/*
 * Runs the protocol client after the socket connection is established.
 * Accepts a socket, negotiates the protocol version, and passes the control
 * to the respective version handler.
 */
extern int handle_proto(socket_t *socket, uint8_t method, const MethodArgs *args, StatusCallback *callback);

#endif  // PROTO_SELECTOR_H_
