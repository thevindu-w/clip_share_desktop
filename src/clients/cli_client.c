/*
 * clients/cli_client.c - methods for CLI client
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

#include <clients/cli_client.h>
#include <clients/udp_scan.h>
#include <globals.h>
#include <proto/selector.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#define COMMAND_SCAN 126
#define COMMAND_GET_TEXT 1
#define COMMAND_SEND_TEXT 2
#define COMMAND_GET_FILES 3
#define COMMAND_SEND_FILES 4
#define COMMAND_GET_IMAGE 5
#define COMMAND_GET_COPIED_IMAGE 6
#define COMMAND_GET_SCREENSHOT 7

static int _invoke_method(uint32_t server_addr, unsigned char method, MethodArgs *args) {
    socket_t sock;
    connect_server(&sock, server_addr);
    if (IS_NULL_SOCK(sock.type)) {
        puts("Couldn't connect");
        return EXIT_FAILURE;
    }
    int ret = handle_proto(&sock, method, args, NULL);
    close_socket_no_wait(&sock);
    return ret;
}

static inline void _get_text(uint32_t server_addr) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_GET_TEXT, NULL) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get text %s\n", msg_suffix);
}

static inline void _send_text(uint32_t server_addr) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_SEND_TEXT, NULL) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Send text %s\n", msg_suffix);
}

static inline void _get_files(uint32_t server_addr) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_GET_FILE, NULL) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get files %s\n", msg_suffix);
}

static inline void _send_files(uint32_t server_addr) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_SEND_FILE, NULL) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Send files %s\n", msg_suffix);
}

static inline void _get_image(uint32_t server_addr) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_GET_IMAGE, NULL) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get image %s\n", msg_suffix);
}

static inline void _get_copied_image(uint32_t server_addr) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_GET_COPIED_IMAGE, NULL) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get copied image %s\n", msg_suffix);
}

static inline void _get_screenshot(uint32_t server_addr, MethodArgs *args) {
    const char *msg_suffix;
    if (_invoke_method(server_addr, METHOD_GET_SCREENSHOT, args) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get screenshot %s\n", msg_suffix);
}

/*
 * Parse command line arguments and set corresponding variables
 */
static inline void _parse_args(int argc, char **argv, int8_t *command_p, uint32_t *server_addr_p, MethodArgs *args) {
    char cmd[4];
    strncpy(cmd, argv[0], 3);
    cmd[3] = 0;
    if (strncmp(cmd, "sc", 3) == 0) {
        *command_p = COMMAND_SCAN;
        return;
    } else if (strncmp(cmd, "g", 2) == 0) {
        *command_p = COMMAND_GET_TEXT;
    } else if (strncmp(cmd, "s", 2) == 0) {
        *command_p = COMMAND_SEND_TEXT;
    } else if (strncmp(cmd, "fg", 3) == 0) {
        *command_p = COMMAND_GET_FILES;
    } else if (strncmp(cmd, "fs", 3) == 0) {
        *command_p = COMMAND_SEND_FILES;
    } else if (strncmp(cmd, "i", 2) == 0) {
        *command_p = COMMAND_GET_IMAGE;
    } else if (strncmp(cmd, "ic", 2) == 0) {
        *command_p = COMMAND_GET_COPIED_IMAGE;
    } else if (strncmp(cmd, "is", 2) == 0) {
        *command_p = COMMAND_GET_SCREENSHOT;
    } else {
        fprintf(stderr, "Invalid command %s\n\n", argv[0]);
        *command_p = 0;
        return;
    }
    if (argc < 2) {
        fprintf(stderr, "Server address not provided\n\n");
        *command_p = 0;
        return;
    }
    if (ipv4_aton(argv[1], server_addr_p) != EXIT_SUCCESS) {
        fprintf(stderr, "Invalid server address %s\n\n", argv[1]);
        *command_p = 0;
        return;
    }
    if (argc < 3) return;
    if (*command_p == COMMAND_GET_SCREENSHOT && argv[2] && argv[2][0]) {
        char *end_ptr = NULL;
        unsigned long long disp64 = strtoull(argv[2], &end_ptr, 10);
        if (!end_ptr || *end_ptr || disp64 >= 65536) {
            fprintf(stderr, "Invalid display value %s\n\n", argv[2]);
            *command_p = 0;
            return;
        }
        args->display = (uint16_t)disp64;
    }
}

void cli_client(int argc, char **argv, const char *prog_name) {
    int8_t command;
    uint32_t server_addr;
    MethodArgs args = {.display = 0};
    _parse_args(argc, argv, &command, &server_addr, &args);

    switch (command) {
        case COMMAND_SCAN: {
            net_scan();
            break;
        }
        case COMMAND_GET_TEXT: {
            _get_text(server_addr);
            break;
        }
        case COMMAND_SEND_TEXT: {
            _send_text(server_addr);
            break;
        }
        case COMMAND_GET_FILES: {
            _get_files(server_addr);
            break;
        }
        case COMMAND_SEND_FILES: {
            _send_files(server_addr);
            break;
        }
        case COMMAND_GET_IMAGE: {
            _get_image(server_addr);
            break;
        }
        case COMMAND_GET_COPIED_IMAGE: {
            _get_copied_image(server_addr);
            break;
        }
        case COMMAND_GET_SCREENSHOT: {
            _get_screenshot(server_addr, &args);
            break;
        }
        default: {
            print_usage(prog_name);
            exit(EXIT_FAILURE);
        }
    }
#ifdef __linux__
    if (!pending_data || fork() > 0) return;
    set_pending_clipboard_item();
#endif
}

void net_scan(void) {
    list2 *servers = udp_scan();
    if (!servers) {
        fprintf(stderr, "Scan failed\n");
        exit(EXIT_FAILURE);
    }
    if (servers->len == 0) {
        fprintf(stderr, "No servers found\n");
        free_list(servers);
        return;
    }
    for (size_t i = 0; i < servers->len; i++) {
        puts(servers->array[i]);
    }
    free_list(servers);
}
