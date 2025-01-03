/*
 * utils/net_utils.h - headers for socket connections
 * Copyright (C) 2022-2024 H. Thevindu J. Wijesekera
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

#ifndef UTILS_NET_UTILS_H_
#define UTILS_NET_UTILS_H_

#include <stdint.h>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
typedef int sock_t;
#elif defined(_WIN32)
typedef SOCKET sock_t;
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/*
 * Converts a ipv4 address in dotted decimal into in_addr_t.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int ipv4_aton(const char *address_str, uint32_t *address_ptr);

extern sock_t connect_server(uint32_t server_addr, uint16_t port);

/*
 * Reads num bytes from the socket into buf.
 * buf should be writable and should have a capacitiy of at least num bytes.
 * Waits until all the bytes are read. If reading failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int read_sock(sock_t sock, char *buf, uint64_t size);

/*
 * Writes num bytes from buf to the socket.
 * At least num bytes of the buf should be readable.
 * Waits until all the bytes are written. If writing failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int write_sock(sock_t sock, const char *buf, uint64_t size);

/*
 * Sends a 64-bit signed integer num to socket as big-endian encoded 8 bytes.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int send_size(sock_t sock, int64_t num);

/*
 * Reads a 64-bit signed integer from socket as big-endian encoded 8 bytes.
 * Stores the value of the read integer in the address given by size_ptr.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int read_size(sock_t sock, int64_t *size_ptr);

extern void close_socket(sock_t sock);

#endif  // UTILS_NET_UTILS_H_
