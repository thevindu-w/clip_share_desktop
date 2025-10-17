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
#include <stdlib.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <sys/socket.h>
#elif defined(_WIN32)
#include <iphlpapi.h>
#include <winsock2.h>
#endif

#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0
#endif

#define MAX_THREADS 16
#define ADDR_BUF_SZ 16
#define CAST_SOCKADDR_IN (struct sockaddr_in *)(void *)

#ifdef _WIN32

typedef int socklen_t;
typedef u_long in_addr_t;
typedef HANDLE mutex_t;

static inline int mutex_lock(mutex_t mutex) { return WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0; }

static inline void mutex_unlock(mutex_t mutex) { ReleaseMutex(mutex); }

#elif defined(__linux__) || defined(__APPLE__)

typedef pthread_mutex_t *mutex_t;

static inline int mutex_lock(mutex_t mutex) { return pthread_mutex_lock(mutex); }

static inline void mutex_unlock(mutex_t mutex) { pthread_mutex_unlock(mutex); }

#endif

typedef struct {
    in_addr_t brd_addr;
    mutex_t mutex;
    list2 *server_lst;
} scan_arg_t;

static void *scan_fn(void *args) {
    scan_arg_t *arg = (scan_arg_t *)args;
    in_addr_t broadcast = arg->brd_addr;
    mutex_t mutex = arg->mutex;
    list2 *servers = arg->server_lst;
    free(arg);

    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(configuration.udp_port);
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
        if (!mutex_lock(mutex)) {
            append(servers, strdup(server));
            mutex_unlock(mutex);
        }
    }
    close_socket_no_wait(&socket);

    return NULL;
}

static int addr_comp(const void *elem1, const void *elem2) {
    const char *addr1 = *(const char *const *)elem1;
    const char *addr2 = *(const char *const *)elem2;
    return strncmp(addr1, addr2, ADDR_BUF_SZ);
}

static list2 *filter_addresses(list2 *servers, list2 *addrs) {
    if (!servers->len) {
        free_list(addrs);
        return servers;
    }

    // Remove address of this client machine
    for (unsigned int i = 0; i < servers->len; i++) {
        char *server = servers->array[i];
        for (unsigned int j = 0; j < addrs->len; j++) {
            if (!strncmp(server, addrs->array[j], ADDR_BUF_SZ)) {
                free(server);
                servers->array[i] = servers->array[servers->len - 1];
                servers->array[servers->len - 1] = NULL;
                servers->len -= 1;
                i--;
                break;
            }
        }
    }
    free_list(addrs);

    // Remove duplicates
    qsort(servers->array, servers->len, sizeof(servers->array[0]), &addr_comp);
    for (unsigned int i = 1; i < servers->len; i++) {
        char *prev = servers->array[i - 1];
        char *cur = servers->array[i];
        if (!strncmp(prev, cur, ADDR_BUF_SZ)) {
            free(cur);
            servers->array[i] = servers->array[servers->len - 1];
            servers->array[servers->len - 1] = NULL;
            servers->len -= 1;
            i--;
        }
    }
    return servers;
}

#if defined(__linux__) || defined(__APPLE__)

list2 *udp_scan(void) {
    struct ifaddrs *ptr_ifaddrs = NULL;
    if (getifaddrs(&ptr_ifaddrs)) {
        return NULL;
    }
    list2 *server_lst = init_list(2);
    list2 *addr_lst = init_list(2);
    char ifaddr[ADDR_BUF_SZ];
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
        struct in_addr addr = (CAST_SOCKADDR_IN(ptr_entry->ifa_addr))->sin_addr;
        if (addr.s_addr == htonl(INADDR_LOOPBACK)) {
            continue;
        }

        if (inet_ntop(AF_INET, &addr, ifaddr, ADDR_BUF_SZ)) {
            ifaddr[ADDR_BUF_SZ - 1] = 0;
            append(addr_lst, strdup(ifaddr));
        }

        pthread_t tid;
        scan_arg_t *arg = malloc(sizeof(scan_arg_t));
        arg->brd_addr = addr.s_addr | ~(CAST_SOCKADDR_IN(ptr_entry->ifa_netmask))->sin_addr.s_addr;
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
    pthread_mutex_destroy(&mutex);
    return filter_addresses(server_lst, addr_lst);
}

#elif defined(_WIN32)

static DWORD WINAPI scan_fn_wrapper(void *arg) {
    scan_fn(arg);
    return EXIT_SUCCESS;
}

list2 *udp_scan(void) {
    ULONG bufSz = 4096;
    PIP_ADAPTER_ADDRESSES pAddrs = NULL;
    int retries = 3;
    do {
        pAddrs = malloc(bufSz);
        if (!pAddrs) {
            return NULL;
        }
        ULONG flags =
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
        ULONG ret = GetAdaptersAddresses(AF_INET, flags, NULL, pAddrs, &bufSz);
        if (ret == NO_ERROR) {
            break;
        }
        free(pAddrs);
        pAddrs = NULL;
        if (ret != ERROR_BUFFER_OVERFLOW) {
            return NULL;
        }
        retries--;
    } while (retries > 0 && bufSz < 1000000L);
    if (!pAddrs) {
        return NULL;
    }

    HANDLE mutex = CreateMutex(NULL, FALSE, NULL);
    if (!mutex) {
        free(pAddrs);
        return NULL;
    }
    list2 *server_lst = init_list(2);
    list2 *addr_lst = init_list(2);
    char ifaddr[ADDR_BUF_SZ];
    HANDLE threads[MAX_THREADS];
    int ind = 0;
    for (PIP_ADAPTER_ADDRESSES cur = pAddrs; cur; cur = cur->Next) {
        PIP_ADAPTER_UNICAST_ADDRESS unicast = cur->FirstUnicastAddress;
        if (!unicast) {
            continue;
        }
        SOCKET_ADDRESS socket_addr = unicast->Address;
        if (socket_addr.lpSockaddr->sa_family != AF_INET) {
            continue;
        }
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif
        struct in_addr addr = (CAST_SOCKADDR_IN(socket_addr.lpSockaddr))->sin_addr;
        if (addr.s_addr == htonl(INADDR_LOOPBACK)) {
            continue;
        }

        if (inet_ntop(AF_INET, &addr, ifaddr, ADDR_BUF_SZ)) {
            ifaddr[ADDR_BUF_SZ - 1] = 0;
            append(addr_lst, strdup(ifaddr));
        }

        in_addr_t mask = ~((1 << unicast->OnLinkPrefixLength) - 1);

        scan_arg_t *arg = malloc(sizeof(scan_arg_t));
        arg->brd_addr = addr.s_addr | mask;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
        arg->mutex = mutex;
        arg->server_lst = server_lst;
        HANDLE thread = CreateThread(NULL, 0, scan_fn_wrapper, arg, 0, NULL);
        if (!thread) {
            free(arg);
            continue;
        }
        threads[ind++] = thread;
        if (ind >= MAX_THREADS) {
            break;
        }
    }
    free(pAddrs);

    for (int i = 0; i < 40; i++) {
        if (server_lst->len) {
            break;
        }
        Sleep(50);
    }
    Sleep(200);

    for (int i = 0; i < ind; i++) {
        TerminateThread(threads[i], 1);
    }
    CloseHandle(mutex);
    return filter_addresses(server_lst, addr_lst);
}

#endif
