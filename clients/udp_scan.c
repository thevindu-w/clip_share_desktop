/*
 * clients/udp_scan.c - UDP scanner
 * Copyright (C) 2024 H. Thevindu J. Wijesekera
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

#include <clients/udp_scan.h>
#include <globals.h>
#include <stdio.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0
#endif

#ifdef _WIN32
typedef int socklen_t;
#endif

list2 *udp_scan(void) {
    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4337);
    uint32_t brd_addr;
    if (ipv4_aton("255.255.255.255", &brd_addr) != EXIT_SUCCESS) {
        return NULL;
    }
    serv_addr.sin_addr.s_addr = brd_addr;

    socket_t socket;
    get_udp_socket(&socket);
    if (IS_NULL_SOCK(socket.type)) {
        return NULL;
    }
    sock_t sock = socket.socket;

    socklen_t len = sizeof(serv_addr);
    sendto(sock, "in", 2, MSG_CONFIRM, (const struct sockaddr *)&serv_addr, len);

    struct timeval tv_connect;
    list2 *serv_lst = init_list(1);
    const int buf_sz = 16;
    char buffer[buf_sz];
    for (int i = 0; i < 1024; i++) {
        int n = (int)recvfrom(sock, (char *)buffer, (size_t)buf_sz, 0, (struct sockaddr *)&serv_addr, &len);
        if (n <= 0) n = 0;
        if (n >= buf_sz) n = buf_sz - 1;
        buffer[n] = '\0';
        if (strncmp(INFO_NAME, buffer, (size_t)buf_sz)) break;
        char *server = inet_ntoa(serv_addr.sin_addr);
        append(serv_lst, strdup(server));
        if (i == 0) {
            // reduce timeout for subsequent recvfrom calls to 200ms
            tv_connect.tv_sec = 0;
            tv_connect.tv_usec = 200000L;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv_connect, sizeof(tv_connect));  // ignore failure
        }
    }
    close_socket_no_wait(&socket);
    return serv_lst;
}
