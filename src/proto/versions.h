/*
 * proto/versions.h - header for declaring protocol versions
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

#ifndef PROTO_VERSIONS_H_
#define PROTO_VERSIONS_H_

#include <clients/status_cb.h>
#include <stdint.h>
#include <utils/net_utils.h>

#if PROTOCOL_MIN <= 1
/*
 * Accepts a socket connection and method code after the protocol version 1 is selected after the negotiation phase.
 * Negotiate the method code with the server and pass the control to the respective method handler.
 */
extern int version_1(socket_t *socket, uint8_t method, StatusCallback *callback);
#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
/*
 * Accepts a socket connection and method code after the protocol version 2 is selected after the negotiation phase.
 * Negotiate the method code with the server and pass the control to the respective method handler.
 */
extern int version_2(socket_t *socket, uint8_t method, StatusCallback *callback);
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
/*
 * Accepts a socket connection and method code after the protocol version 3 is selected after the negotiation phase.
 * Negotiate the method code with the server and pass the control to the respective method handler.
 */
extern int version_3(socket_t *socket, uint8_t method, MethodArgs *args, StatusCallback *callback);
#endif

#endif  // PROTO_VERSIONS_H_
