/*
 * proto/versions.c - platform independent implementation of protocol versions
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

#include <proto/methods.h>
#include <proto/selector.h>
#include <proto/versions.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/net_utils.h>

// status codes
#define STATUS_UNKNOWN_METHOD 3
#define STATUS_METHOD_NOT_IMPLEMENTED 4

#if PROTOCOL_MIN <= 1

int version_1(sock_t socket, unsigned char method) {
    switch (method) {
        case METHOD_GET_TEXT:
        case METHOD_SEND_TEXT:
        case METHOD_GET_FILE:
        case METHOD_SEND_FILE:
        case METHOD_GET_IMAGE:
        case METHOD_INFO:
            break;  // valid method

        default: {  // unknown method
#ifdef DEBUG_MODE
            fprintf(stderr, "Unknown method for version 1\n");
#endif
            return EXIT_FAILURE;
        }
    }

    if (write_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    unsigned char method_status;
    if (read_sock(socket, (char *)&method_status, 1) != EXIT_SUCCESS || method_status != STATUS_OK) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Method selection failed\n");
#endif
        return EXIT_FAILURE;
    }

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket);
        }
        case METHOD_GET_FILE: {
            return get_files_v1(socket);
        }
        case METHOD_SEND_FILE: {
            return send_file_v1(socket);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)

int version_2(sock_t socket, unsigned char method) {
    switch (method) {
        case METHOD_GET_TEXT:
        case METHOD_SEND_TEXT:
        case METHOD_GET_FILE:
        case METHOD_SEND_FILE:
        case METHOD_GET_IMAGE:
        case METHOD_INFO:
            break;  // valid method

        default: {  // unknown method
#ifdef DEBUG_MODE
            fprintf(stderr, "Unknown method for version 2\n");
#endif
            return EXIT_FAILURE;
        }
    }

    if (write_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    unsigned char method_status;
    if (read_sock(socket, (char *)&method_status, 1) != EXIT_SUCCESS || method_status != STATUS_OK) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Method selection failed\n");
#endif
        return EXIT_FAILURE;
    }

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket);
        }
        case METHOD_GET_FILE: {
            return get_files_v2(socket);
        }
        case METHOD_SEND_FILE: {
            return send_files_v2(socket);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)

int version_3(sock_t socket, unsigned char method) {
    switch (method) {
        case METHOD_GET_TEXT:
        case METHOD_SEND_TEXT:
        case METHOD_GET_FILE:
        case METHOD_SEND_FILE:
        case METHOD_GET_IMAGE:
        case METHOD_GET_COPIED_IMAGE:
        case METHOD_GET_SCREENSHOT:
        case METHOD_INFO:
            break;  // valid method

        default: {  // unknown method
#ifdef DEBUG_MODE
            fprintf(stderr, "Unknown method for version 3\n");
#endif
            return EXIT_FAILURE;
        }
    }

    if (write_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    unsigned char method_status;
    if (read_sock(socket, (char *)&method_status, 1) != EXIT_SUCCESS || method_status != STATUS_OK) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Method selection failed\n");
#endif
        return EXIT_FAILURE;
    }

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket);
        }
        case METHOD_GET_FILE: {
            return get_files_v3(socket);
        }
        case METHOD_SEND_FILE: {
            return send_files_v3(socket);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket);
        }
        case METHOD_GET_COPIED_IMAGE: {
            return get_copied_image_v3(socket);
        }
        case METHOD_GET_SCREENSHOT: {
            return get_screenshot_v3(socket);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#endif
