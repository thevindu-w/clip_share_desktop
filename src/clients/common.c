/*
 * clients/common.c - methods common for all clients
 * Copyright (C) 2026 H. Thevindu J. Wijesekera
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

#include <clients/common.h>
#include <proto/selector.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>
#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#elif defined(_WIN32)
#include <processthreadsapi.h>
#include <synchapi.h>
#endif

#define MAX_THREADS 128

#ifdef _WIN32
typedef HANDLE thread_t;
#elif defined(__linux__) || defined(__APPLE__)
typedef pthread_t thread_t;
#endif

typedef struct {
    char *server;
    char sep;
    char *out;
} arg_t;

static void *get_name(void *args) {
    arg_t *arg = (arg_t *)args;
    char *server = arg->server;
    socket_t sock;
    uint32_t server_addr;
    if (ipv4_aton(server, &server_addr) != EXIT_SUCCESS) {
        return NULL;
    }
    connect_server(&sock, server_addr);
    if (IS_NULL_SOCK(sock.type)) {
        return NULL;
    }
    char *data = NULL;
    MethodArgs method_args = {0};
    method_args.data_p = &data;
    int ret = handle_proto(&sock, METHOD_INFO, &method_args, NULL);
    close_socket_no_wait(&sock);
    if (ret != EXIT_SUCCESS) {
        if (data) {
            free(data);
        }
        return NULL;
    }
    char *line = strstr(data, "\nserver_name=");
    if (!line) {
        if (data) {
            free(data);
        }
        return NULL;
    }
    line += 13;
    char *eol = strchr(line, '\n');
    if (eol) {
        *eol = '\0';
    }
    size_t len = strnlen(line, 512);
    memmove(data, line, len);
    data[len] = 0;
    size_t addr_len = strnlen(server, 128);
    size_t tot_len = addr_len + len + 2;
    char *out = malloc(tot_len);
    if (!out) {
        if (data) {
            free(data);
        }
        return NULL;
    }
    snprintf(out, tot_len, "%s%c%s", server, arg->sep, data);
    free(data);
    arg->out = out;
    return NULL;
}

#ifdef _WIN32

static DWORD WINAPI get_name_wrapper(void *arg) {
    get_name(arg);
    return EXIT_SUCCESS;
}

#endif

void get_server_names(list2 *servers, char sep) {
    if (!servers) {
        return;
    }
    arg_t args[MAX_THREADS];
    thread_t threads[MAX_THREADS];
    for (unsigned int i = 0; i < servers->len && i < MAX_THREADS; i++) {
        char *server = servers->array[i];
        arg_t arg = {.server = server, .sep = sep, .out = NULL};
        args[i] = arg;
        thread_t thread;
#ifdef _WIN32
        thread = CreateThread(NULL, 0, get_name_wrapper, &(args[i]), 0, NULL);
#elif defined(__linux__) || defined(__APPLE__)
        thread = 0;
        if (pthread_create(&thread, NULL, &get_name, &(args[i]))) {
            thread = 0;
        }
#endif
        threads[i] = thread;
    }
    for (unsigned int i = 0; i < servers->len && i < MAX_THREADS; i++) {
#ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
#elif defined(__linux__) || defined(__APPLE__)
        pthread_join(threads[i], NULL);
#endif
        char *out = args[i].out;
        if (out) {
            free(servers->array[i]);
            servers->array[i] = (char *)out;
        }
    }
}
