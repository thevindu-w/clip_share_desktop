/*
 * clients/gui_client.c - methods for web client
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

#include <clients/gui_client.h>
#include <clients/status_cb.h>
#include <globals.h>
#include <microhttpd.h>
#include <proto/selector.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utils/net_utils.h>

#define PORT 8888

extern char _binary_blob_page_html_start[];
extern char _binary_blob_page_html_end[];
static size_t _page_size;

#define page_blob _binary_blob_page_html_start
#define page_blob_end _binary_blob_page_html_end

static const char *CONTENT_TYPE_TEXT = "text/plain";
static const char *CONTENT_TYPE_HTML = "text/html";

static int pipedes[2];

typedef struct _get_query_params {
    char *server;
} get_query_params;

static void callback_fn(unsigned int status, const char *msg, size_t len, status_callback_params *params) {
    if (!params || params->called) return;
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

static void handle_method(struct MHD_Connection *connection, const char *address, uint8_t method) {
    status_callback_params params = {.called = 0, .connection = connection};
    uint32_t server_addr;
    if (ipv4_aton(address, &server_addr) != EXIT_SUCCESS) {
        callback_fn(INVALID_ADDRESS, NULL, 0, &params);
        return;
    }
    sock_t sock = connect_server(server_addr, configuration.app_port);
    if (sock == INVALID_SOCKET) {
        callback_fn(CONNECTION_FAILURE, NULL, 0, &params);
        return;
    }
    StatusCallback callback = {.function = &callback_fn, .params = &params};
    handle_proto(sock, method, &callback);
    close_socket(sock);
    callback_fn(LOCAL_ERROR, NULL, 0, &params);
}

static int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
    (void)kind;
    get_query_params *query = (get_query_params *)cls;
    if (!strcmp(key, "server")) {
        query->server = strndup(value, 17);
    }
#ifdef DEBUG_MODE
    else {
        printf("%s: %s\n", key, value);
    }
#endif
    return MHD_YES;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                                const char *version, const char *upload_data, size_t *upload_data_size,
                                void **con_cls) {
    struct MHD_Response *response;
    (void)cls;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    char empty_resp[] = "";

    unsigned int res_status = MHD_HTTP_OK;
    if (!strcmp(method, MHD_HTTP_METHOD_GET)) {
        response = MHD_create_response_from_buffer(_page_size, (void *)page_blob, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", CONTENT_TYPE_HTML);
    } else if (!strcmp(method, MHD_HTTP_METHOD_POST)) {
        get_query_params query = {.server = NULL};
        MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, print_out_key, &query);
        if (!query.server) {
            response = MHD_create_response_from_buffer(0, (void *)empty_resp, MHD_RESPMEM_PERSISTENT);
            res_status = MHD_HTTP_BAD_REQUEST;
        } else if (!strcmp(url, "/get/text")) {
            handle_method(connection, query.server, METHOD_GET_TEXT);
            return MHD_YES;
        } else if (!strcmp(url, "/get/file")) {
            handle_method(connection, query.server, METHOD_GET_FILE);
            return MHD_YES;
        } else if (!strcmp(url, "/get/image")) {
            handle_method(connection, query.server, METHOD_GET_IMAGE);
            return MHD_YES;
        } else if (!strcmp(url, "/send/text")) {
            handle_method(connection, query.server, METHOD_SEND_TEXT);
            return MHD_YES;
        } else if (!strcmp(url, "/send/file")) {
            handle_method(connection, query.server, METHOD_SEND_FILE);
            return MHD_YES;
        } else {
            response = MHD_create_response_from_buffer(0, (void *)empty_resp, MHD_RESPMEM_PERSISTENT);
            res_status = MHD_HTTP_NOT_FOUND;
        }
        if (query.server) free(query.server);
    } else {
        response = MHD_create_response_from_buffer(0, (void *)empty_resp, MHD_RESPMEM_PERSISTENT);
        res_status = MHD_HTTP_NOT_IMPLEMENTED;
    }
    int ret = MHD_queue_response(connection, res_status, response);
    MHD_destroy_response(response);

    return ret;
}

void start_web(void) {
    _page_size = (size_t)(page_blob_end - page_blob);

    struct MHD_Daemon *daemon;

    if (pipe(pipedes)) return;
    daemon = MHD_start_daemon(MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL, &answer_to_connection,
                              NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        close(pipedes[0]);
        close(pipedes[1]);
        return;
    }

    char c;
    ssize_t rd = read(pipedes[0], &c, 1);
    if (rd != 1) puts("Read failed");
    puts("End");

    MHD_stop_daemon(daemon);
    return;
}