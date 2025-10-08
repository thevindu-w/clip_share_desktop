/*
 * clients/udp_scan.c - UDP scanner
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

#include <clients/udp_scan.h>
#include <globals.h>
#include <stdio.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pthread.h>
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

#define MAX_THREADS 16
#define ADDR_BUF_SZ 16
#define CAST_SOCKADDR_IN (struct sockaddr_in *)(void *)

typedef struct {
    in_addr_t brd_addr;
    pthread_mutex_t *mutex;
    list2 *server_lst;
} scan_arg_t;

static void *scan_fn(void *args) {
    scan_arg_t *arg = (scan_arg_t *)args;
    in_addr_t broadcast = arg->brd_addr;
    pthread_mutex_t *mutex = arg->mutex;
    list2 *servers = arg->server_lst;
    free(arg);

    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4337);
    server_addr.sin_addr.s_addr = broadcast;

    socket_t socket;
    get_udp_socket(&socket);
    if (IS_NULL_SOCK(socket.type)) {
        return NULL;
    }
    sock_t sock = socket.socket.plain;

    socklen_t len = sizeof(server_addr);
    sendto(sock, "in", 2, MSG_CONFIRM, (const struct sockaddr *)&server_addr, len);

    const int buf_sz = 16;
    char buffer[buf_sz];
    for (int i = 0; i < 256; i++) {
        int n = (int)recvfrom(sock, (char *)buffer, (size_t)buf_sz, 0, (struct sockaddr *)&server_addr, &len);
        if (n < 0) {
            n = 0;
        } else if (n >= buf_sz) {
            n = buf_sz - 1;
        }
        buffer[n] = '\0';
        if (strncmp(INFO_NAME, buffer, (size_t)buf_sz)) {
#ifdef DEBUG_MODE
            puts("Incorrect scan response");
#endif
            continue;
        }

        char server[ADDR_BUF_SZ];
        if (!inet_ntop(AF_INET, &server_addr.sin_addr, server, ADDR_BUF_SZ)) {
#ifdef DEBUG_MODE
            puts("inet_ntop failed");
#endif
            continue;
        }
        server[ADDR_BUF_SZ - 1] = 0;
#ifdef DEBUG_MODE
        char broadcast_str[ADDR_BUF_SZ];
        struct in_addr broadcast_addr = {.s_addr = broadcast};
        inet_ntop(AF_INET, &broadcast_addr, broadcast_str, ADDR_BUF_SZ);
        printf("Server %s from %s\n", server, broadcast_str);
#endif
        if (!pthread_mutex_lock(mutex)) {
            append(servers, strdup(server));
            pthread_mutex_unlock(mutex);
        }
    }
    close_socket_no_wait(&socket);

    return NULL;
}

list2 *udp_scan(void) {
    struct ifaddrs *ptr_ifaddrs = NULL;
    if (getifaddrs(&ptr_ifaddrs)) {
        return NULL;
    }
    list2 *server_lst = init_list(2);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t threads[MAX_THREADS];
    int ind = 0;
    for (struct ifaddrs *ptr_entry = ptr_ifaddrs; ptr_entry; ptr_entry = ptr_entry->ifa_next) {
        if (!(ptr_entry->ifa_addr) || ptr_entry->ifa_addr->sa_family != AF_INET) {
            continue;
        }
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif
        in_addr_t addr = (CAST_SOCKADDR_IN(ptr_entry->ifa_addr))->sin_addr.s_addr;
        if (addr == htonl(INADDR_LOOPBACK)) {
            continue;
        }

        pthread_t tid;
        scan_arg_t *arg = malloc(sizeof(scan_arg_t));
        arg->brd_addr = addr | ~(CAST_SOCKADDR_IN(ptr_entry->ifa_netmask))->sin_addr.s_addr;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        arg->mutex = &mutex;
        arg->server_lst = server_lst;
        if (pthread_create(&tid, NULL, &scan_fn, arg)) {
            free(arg);
            continue;
        }
        threads[ind++] = tid;
        if (ind >= MAX_THREADS) {
            break;
        }
    }
    freeifaddrs(ptr_ifaddrs);

    struct timespec interval = {.tv_sec = 0, .tv_nsec = 50000000L};
    for (int i = 0; i < 40; i++) {
        if (server_lst->len) {
            break;
        }
        nanosleep(&interval, NULL);
    }
    interval.tv_nsec = 200000000;
    nanosleep(&interval, NULL);

    for (int i = 0; i < ind; i++) {
        pthread_cancel(threads[i]);
    }

    return server_lst;
}
