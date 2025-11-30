/*
 * utils/clipboard_listener.h - implementation for clipboard listener
 * Copyright (C) 2025 H. Thevindu J. Wijesekera
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
#include <proto/selector.h>
#include <utils/clipboard_listener.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#endif

typedef struct {
    char *server;
    int type;
} send_arg_t;

static void *send_to_server(void *args) {
    send_arg_t *arg = (send_arg_t *)args;
    const char *server = arg->server;
    int type = arg->type;
    free(arg);

    uint32_t server_addr;
    if (ipv4_aton(server, &server_addr) != EXIT_SUCCESS) {
        return NULL;
    }
    socket_t sock;
    connect_server(&sock, server_addr);
    if (IS_NULL_SOCK(sock.type)) {
        return NULL;
    }
    uint8_t method = (type == COPIED_TYPE_FILE) ? METHOD_SEND_FILE : METHOD_SEND_TEXT;
    MethodArgs methodArgs = {0};
    methodArgs.is_auto_send = 1;
    handle_proto(&sock, method, &methodArgs, NULL);
    close_socket_no_wait(&sock);
    return NULL;
}

#ifdef _WIN32
static DWORD WINAPI send_to_server_wrapper(void *arg) {
    send_to_server(arg);
    return EXIT_SUCCESS;
}
#endif

static void send_to_servers(int type) {
    switch (type) {
        case COPIED_TYPE_TEXT: {
            if (!configuration.auto_send_text) {
                return;
            }
            break;
        }
        case COPIED_TYPE_FILE: {
            if (!configuration.auto_send_files) {
                return;
            }
            break;
        }
        default:
            return;
    }
    list2 *servers = udp_scan();
    if (!servers) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Scan failed\n");
#endif
        return;
    }
    if (servers->len <= 0 || servers->len > 512) {
#ifdef DEBUG_MODE
        fprintf(stderr, "No servers or too many servers found\n");
#endif
        free_list(servers);
        return;
    }

#ifdef _WIN32
    HANDLE threads[servers->len];
#elif defined(__linux__) || defined(__APPLE__)
    pthread_t threads[servers->len];
#endif
    for (size_t i = 0; i < servers->len; i++) {
        send_arg_t *arg = malloc(sizeof(send_arg_t));
        arg->server = servers->array[i];
        arg->type = type;
#ifdef _WIN32
        HANDLE thread = CreateThread(NULL, 0, send_to_server_wrapper, arg, 0, NULL);
        if (!thread) {
            free(arg);
        }
        threads[i] = thread;
#elif defined(__linux__) || defined(__APPLE__)
        pthread_t tid = 0;
        if (pthread_create(&tid, NULL, &send_to_server, arg)) {
            free(arg);
        }
        threads[i] = tid;
#endif
    }
    for (size_t i = 0; i < servers->len; i++) {
        if (!threads[i]) {
            continue;
        }
#ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
#elif defined(__linux__) || defined(__APPLE__)
        pthread_join(threads[i], NULL);
#endif
    }
    free_list(servers);
}

int start_clipboard_listener(void) { return clipboard_listen(&send_to_servers); }
