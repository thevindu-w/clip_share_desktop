/*
 * utils/net_utils.h - headers for socket connections
 * Copyright (C) 2022-2025 H. Thevindu J. Wijesekera
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
#ifndef NO_SSL
#include <openssl/ssl.h>
#endif
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

// Connection validity mask
#define MASK_VALID 0x1
#define NULL_SOCK 0x0
#define VALID_SOCK 0x1
#define IS_NULL_SOCK(type) ((type & MASK_VALID) == NULL_SOCK)  // NOLINT(runtime/references)

// Connection encryption mask
#define MASK_ENC 0x2
#define PLAIN_SOCK 0x0
#define SSL_SOCK 0x2
#define IS_SSL(type) ((type & MASK_ENC) == SSL_SOCK)  // NOLINT(runtime/references)

// Transport layer protocol mask
#define MASK_TRNSPRT_PROTO 0x4
#define TRNSPRT_TCP 0x0
#define TRNSPRT_UDP 0x4
#define IS_UDP(type) ((type & MASK_TRNSPRT_PROTO) == TRNSPRT_UDP)  // NOLINT(runtime/references)

typedef struct _socket_t {
    union {
        sock_t plain;
#ifndef NO_SSL
        SSL *ssl;
#endif
    } socket;
    unsigned char type;
} socket_t;

#ifndef NO_SSL
extern void clear_ssl_ctx(void);
#endif

/*
 * Checks if the address_str is a valid IPv4 address in dot-decimal notation.
 * returns zero for valid addresses and non-zero for invalid addresses.
 */
extern int validate_ipv4(const char *address_str);

/*
 * Converts a ipv4 address in dotted decimal into in_addr_t.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int ipv4_aton(const char *address_str, uint32_t *address_ptr);

extern void connect_server(socket_t *socket, uint32_t server_addr);

extern void get_udp_socket(socket_t *sock_p);

/*
 * Reads num bytes from the socket into buf.
 * buf should be writable and should have a capacitiy of at least num bytes.
 * Waits until all the bytes are read. If reading failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int read_sock(socket_t *socket, char *buf, uint64_t size);

/*
 * Writes num bytes from buf to the socket.
 * At least num bytes of the buf should be readable.
 * Waits until all the bytes are written. If writing failed before num bytes, returns EXIT_FAILURE
 * Otherwise, returns EXIT_SUCCESS.
 */
extern int write_sock(socket_t *socket, const char *buf, uint64_t size);

/*
 * Sends a 64-bit signed integer num to socket as big-endian encoded 8 bytes.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int send_size(socket_t *socket, int64_t num);

/*
 * Reads a 64-bit signed integer from socket as big-endian encoded 8 bytes.
 * Stores the value of the read integer in the address given by size_ptr.
 * returns EXIT_SUCCESS on success. Otherwise, returns EXIT_FAILURE on error.
 */
extern int read_size(socket_t *socket, int64_t *size_ptr);

/*
 * Closes a socket.
 */
extern void _close_socket(socket_t *socket, int await);

#define close_socket(socket) _close_socket(socket, 1)
#define close_socket_no_wait(socket) _close_socket(socket, 0)

#endif  // UTILS_NET_UTILS_H_
