/*
 * client.c - methods for client
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

#include <client.h>
#include <globals.h>
#include <proto/selector.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>

static int _invoke_method(unsigned char method) {
    sock_t sock = connect_server(server_addr, configuration.app_port);
    if (sock == INVALID_SOCKET) {
        puts("Couldn't connect");
        return EXIT_FAILURE;
    }
    int ret = handle_proto(sock, method);
    close_socket(sock);
    return ret;
}

void get_text(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_TEXT) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get text %s\n", msg_suffix);
}

void send_text(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_SEND_TEXT) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Send text %s\n", msg_suffix);
}

void get_files(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_FILE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get files %s\n", msg_suffix);
}

void send_files(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_SEND_FILE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Send files %s\n", msg_suffix);
}

void get_image(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_IMAGE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get image %s\n", msg_suffix);
}

void get_copied_image(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_COPIED_IMAGE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get copied image %s\n", msg_suffix);
}

void get_screenshot(void) {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_SCREENSHOT) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get screenshot %s\n", msg_suffix);
}
