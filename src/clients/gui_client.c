/*
 * clients/gui_client.c - methods for web client
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

#ifndef NO_WEB

#include <clients/gui_client.h>
#include <clients/status_cb.h>
#include <clients/udp_scan.h>
#include <globals.h>
#include <microhttpd.h>
#include <proto/selector.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#ifdef __linux__
#include <pthread.h>
#include <sys/wait.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

struct MHD_Daemon *http_daemon = NULL;

extern char page_html[];
extern unsigned int page_html_len;

static const char *CONTENT_TYPE_TEXT = "text/plain";
static const char *CONTENT_TYPE_HTML = "text/html";

typedef struct _get_query_params {
    char *server;
    int32_t display;
} get_query_params;

#ifdef MHD_YES
typedef int MHD_Result_t;
#else
typedef enum MHD_Result MHD_Result_t;
#endif

#ifdef __linux__
static void *set_clipboard_thread_fn(void *args) {
    (void)args;
    set_pending_clipboard_item();
    return NULL;
}
#endif

static void callback_fn(unsigned int status, const char *msg, size_t len, status_callback_params *params) {
    if ((!params) || params->called) {
        return;
    }
    params->called = 1;
    if (!msg) msg = "";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    struct MHD_Response *response = MHD_create_response_from_buffer(len, (void *)msg, MHD_RESPMEM_MUST_COPY);
#pragma GCC diagnostic pop
    MHD_add_response_header(response, "Content-Type", CONTENT_TYPE_TEXT);
    MHD_queue_response(params->connection, status, response);
    MHD_destroy_response(response);
}

static void handle_method(struct MHD_Connection *connection, const char *address, uint8_t method, MethodArgs *args) {
    status_callback_params params = {.called = 0, .connection = connection};
    switch (method) {
        case METHOD_SEND_TEXT: {
            if (get_copied_type() != COPIED_TYPE_TEXT) {
                callback_fn(RESP_NO_DATA, NULL, 0, &params);
                return;
            }
            break;
        }

        case METHOD_SEND_FILE: {
            if (get_copied_type() != COPIED_TYPE_FILE) {
                callback_fn(RESP_NO_DATA, NULL, 0, &params);
                return;
            }
            break;
        }

        default:
            break;
    }
    uint32_t server_addr;
    if (ipv4_aton(address, &server_addr) != EXIT_SUCCESS) {
        callback_fn(RESP_INVALID_ADDRESS, NULL, 0, &params);
        return;
    }
    socket_t sock;
    connect_server(&sock, server_addr);
    if (IS_NULL_SOCK(sock.type)) {
        callback_fn(RESP_CONNECTION_FAILURE, NULL, 0, &params);
        return;
    }
    StatusCallback callback = {.function = &callback_fn, .params = &params};
    handle_proto(&sock, method, args, &callback);
    close_socket_no_wait(&sock);
    callback_fn(RESP_LOCAL_ERROR, NULL, 0, &params);
}

static void handle_scan(struct MHD_Connection *connection) {
    status_callback_params params = {.called = 0, .connection = connection};
    list2 *server_list = udp_scan();
    if ((!server_list) || server_list->len < 1) {
        if (server_list) free_list(server_list);
        callback_fn(RESP_OK, "", 0, &params);
        return;
    }
    size_t tot_len = 0;
    for (uint32_t i = 0; i < server_list->len; i++) {
        const char *addr = server_list->array[i];
        if (!addr) continue;
        tot_len += strnlen(addr, 64) + 1;
    }
    char *servers = tot_len ? malloc(tot_len) : NULL;
    if (!servers) {
        free_list(server_list);
        callback_fn(RESP_OK, "", 0, &params);
        return;
    }
    char *ptr = servers;
    for (uint32_t i = 0; i < server_list->len; i++) {
        const char *addr = server_list->array[i];
        if (!addr) continue;
        size_t len = strnlen(addr, 64);
        strncpy(ptr, addr, len);
        ptr += len;
        *ptr = ',';
        ptr++;
    }
    ptr--;
    *ptr = '\0';
    free_list(server_list);
    callback_fn(RESP_OK, servers, tot_len - 1, &params);
}

static MHD_Result_t extract_query_params(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    (void)kind;
    get_query_params *query = (get_query_params *)cls;
#ifdef DEBUG_MODE
    printf("%s: %s\n", key, value);
#endif
    if (!strcmp(key, "server")) {
        if (strnlen(value, 17) < 16) {
            query->server = strdup(value);
        }
    } else if ((!strcmp(key, "display")) && value && *value && strnlen(value, 7) < 6) {
        char *end_ptr = NULL;
        unsigned long long disp64 = strtoull(value, &end_ptr, 10);
        if (end_ptr && !*end_ptr && disp64 < 65536) {
            query->display = (int32_t)disp64;
        } else {
            query->display = -1;
#ifdef DEBUG_MODE
            printf("Invalid display %s\n", value);
#endif
        }
    }
    return MHD_YES;
}

static MHD_Result_t answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url,
                                         const char *method, const char *version, const char *upload_data,
                                         size_t *upload_data_size, void **con_cls) {
    struct MHD_Response *response;
    (void)cls;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    char empty_resp[] = "";

    unsigned int res_status = MHD_HTTP_OK;
    if (!strcmp(method, MHD_HTTP_METHOD_GET)) {
        response = MHD_create_response_from_buffer(page_html_len, (void *)page_html, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", CONTENT_TYPE_HTML);
    } else if (!strcmp(method, MHD_HTTP_METHOD_POST)) {
        get_query_params query = {.server = NULL, .display = 0};
        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, extract_query_params, &query);
        char handled = 0;
        if (!strcmp(url, "/scan")) {
            handle_scan(connection);
            handled = 1;
        } else if ((!query.server) || query.display < 0) {
            response = MHD_create_response_from_buffer(0, (void *)empty_resp, MHD_RESPMEM_PERSISTENT);
            res_status = MHD_HTTP_BAD_REQUEST;
        } else if (!strcmp(url, "/get/text")) {
            handle_method(connection, query.server, METHOD_GET_TEXT, NULL);
            handled = 1;
        } else if (!strcmp(url, "/get/file")) {
            handle_method(connection, query.server, METHOD_GET_FILE, NULL);
            handled = 1;
        } else if (!strcmp(url, "/get/image")) {
            handle_method(connection, query.server, METHOD_GET_IMAGE, NULL);
            handled = 1;
        } else if (!strcmp(url, "/get/copied-image")) {
            handle_method(connection, query.server, METHOD_GET_COPIED_IMAGE, NULL);
            handled = 1;
        } else if (!strcmp(url, "/get/screenshot")) {
            MethodArgs args = {0};
            args.display = (uint16_t)query.display;
            handle_method(connection, query.server, METHOD_GET_SCREENSHOT, &args);
            handled = 1;
        } else if (!strcmp(url, "/send/text")) {
            handle_method(connection, query.server, METHOD_SEND_TEXT, NULL);
            handled = 1;
        } else if (!strcmp(url, "/send/file")) {
            handle_method(connection, query.server, METHOD_SEND_FILE, NULL);
            handled = 1;
        } else {
            response = MHD_create_response_from_buffer(0, (void *)empty_resp, MHD_RESPMEM_PERSISTENT);
            res_status = MHD_HTTP_NOT_FOUND;
        }
        if (query.server) free(query.server);
#ifdef __linux__
        if (pending_data) {
            pthread_t pt;
            pthread_create(&pt, NULL, &set_clipboard_thread_fn, NULL);
        }
#endif
        if (handled) return MHD_YES;
    } else {
        response = MHD_create_response_from_buffer(0, (void *)empty_resp, MHD_RESPMEM_PERSISTENT);
        res_status = MHD_HTTP_NOT_IMPLEMENTED;
    }
    MHD_Result_t ret = MHD_queue_response(connection, res_status, response);
    MHD_destroy_response(response);

    return ret;
}

void start_web(void) {
#if defined(__linux__) || defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);  // MHD requires ignoring SIGPIPE
#endif

    struct sockaddr_in bind_addr;
    memset((char *)&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(configuration.ports.web);
    bind_addr.sin_addr.s_addr = configuration.bind_addr;

    http_daemon = MHD_start_daemon(MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, configuration.ports.web, NULL, NULL,
                                   &answer_to_connection, NULL, MHD_OPTION_SOCK_ADDR, &bind_addr, MHD_OPTION_END);
    if (!http_daemon) return;

#if defined(__linux__) || defined(__APPLE__)
    pause();
#elif defined(_WIN32)
    Sleep(INFINITE);
#endif
    return;
}

#endif
