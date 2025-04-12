/*
 * utils/net_utils.c - platform specific implementation for socket connections
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

#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>
#include <utils/utils.h>
#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#define close_sock(sock) close(sock)
#elif defined(_WIN32)
#define close_sock(sock) closesocket(sock);
#endif

int ipv4_aton(const char *address_str, uint32_t *address_ptr) {
    if (!address_ptr || !address_str) return EXIT_FAILURE;
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
    if (sscanf(address_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a >= 256 || b >= 256 || c >= 256 || d >= 256) {
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
    struct in_addr addr;
#if defined(__linux__) || defined(__APPLE__)
    if (inet_aton(address_str, &addr) != 1) {
#elif defined(_WIN32)
    if ((addr.s_addr = inet_addr(address_str)) == INADDR_NONE) {
#endif
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
#if defined(__linux__) || defined(__APPLE__)
    *address_ptr = addr.s_addr;
#elif defined(_WIN32)
    *address_ptr = (uint32_t)addr.s_addr;
#endif
    return EXIT_SUCCESS;
}

void connect_server(uint32_t addr, uint16_t port, socket_t *sock_p) {
    sock_p->socket = 0;
    sock_p->type = NULL_SOCK;
    sock_t sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
#ifdef DEBUG_MODE
        fputs("Can\'t open socket\n", stderr);
#endif
        return;
    }
    // set timeout option to 5s
#if defined(__linux__) || defined(__APPLE__)
    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
#elif defined(_WIN32)
    DWORD timeout = 5000;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))) {
        close_sock(sock);
        error("Can't set the timeout option of the connection");
        return;
    }

    struct sockaddr_in s_addr_in;
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = addr;
    s_addr_in.sin_port = htons(port);

    int status = connect(sock, (struct sockaddr *)&s_addr_in, sizeof(s_addr_in));
    if (status < 0) {
#ifdef DEBUG_MODE
        fputs("Connection failed\n", stderr);
#endif
        close_sock(sock);
        return;
    }

    // set timeout option to 0.5s
#if defined(__linux__) || defined(__APPLE__)
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000L;
#elif defined(_WIN32)
    timeout = 500;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))) {
        close_sock(sock);
        error("Can't set the timeout option of the connection");
        return;
    }
    sock_p->socket = sock;
    sock_p->type = VALID_SOCK;
    return;
}

void get_udp_socket(socket_t *sock_p) {
    sock_p->socket = 0;
    sock_p->type = NULL_SOCK;
    sock_t sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        return;
    }
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast))) {
        close_sock(sock);
        return;
    }
    // set timeout option to 2s
#if defined(__linux__) || defined(__APPLE__)
    struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
#elif defined(_WIN32)
    DWORD timeout = 2000;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))) {
        close_sock(sock);
        return;
    }
    sock_p->socket = sock;
    sock_p->type = VALID_SOCK | TRNSPRT_UDP;
}

int read_sock(socket_t *socket, char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_sz_read = 0;
    char *ptr = buf;
    while (total_sz_read < size) {
        ssize_t sz_read;
        uint64_t read_req_sz = size - total_sz_read;
        if (read_req_sz > 0x7FFFFFFFL) read_req_sz = 0x7FFFFFFFL;  // prevent overflow due to casting
#ifdef _WIN32
        sz_read = recv(socket->socket, ptr, (int)read_req_sz, 0);
#else
        sz_read = recv(socket->socket, ptr, read_req_sz, 0);
#endif
        if (sz_read > 0) {
            total_sz_read += (uint64_t)sz_read;
            cnt = 0;
            ptr += sz_read;
        } else if (cnt++ > 10) {
#ifdef DEBUG_MODE
            fputs("Read sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int write_sock(socket_t *socket, const char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_written = 0;
    const char *ptr = buf;
    while (total_written < size) {
        ssize_t sz_written;
        uint64_t write_req_sz = size - total_written;
        if (write_req_sz > 0x7FFFFFFFL) write_req_sz = 0x7FFFFFFFL;  // prevent overflow due to casting
#ifdef _WIN32
        sz_written = send(socket->socket, ptr, (int)write_req_sz, 0);
#else
        sz_written = send(socket->socket, ptr, write_req_sz, 0);
#endif
        if (sz_written > 0) {
            total_written += (uint64_t)sz_written;
            cnt = 0;
            ptr += sz_written;
        } else if (cnt++ > 10) {
#ifdef DEBUG_MODE
            fputs("Write sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int send_size(socket_t *socket, int64_t size) {
    char sz_buf[8];
    int64_t sz = size;
    for (int i = sizeof(sz_buf) - 1; i >= 0; i--) {
        sz_buf[i] = (char)(sz & 0xff);
        sz >>= 8;
    }
    return write_sock(socket, sz_buf, sizeof(sz_buf));
}

int read_size(socket_t *socket, int64_t *size_ptr) {
    unsigned char sz_buf[8];
    if (read_sock(socket, (char *)sz_buf, sizeof(sz_buf)) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Read size failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    int64_t size = 0;
    for (unsigned i = 0; i < sizeof(sz_buf); i++) {
        size = (size << 8) | sz_buf[i];
    }
    *size_ptr = size;
    return EXIT_SUCCESS;
}

void _close_socket(socket_t *socket, int await) {
    if (IS_NULL_SOCK(socket->type)) return;
    if (await) {
        char tmp;
        recv(socket->socket, &tmp, 1, 0);
    }
    close_sock(socket->socket);
    socket->type = NULL_SOCK;
}
