/*
 * proto/versions.c - platform independent implementation of protocol versions
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

static inline int method_request(socket_t *socket, uint8_t method, StatusCallback *callback) {
    if (write_sock(socket, (char *)&method, 1) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    uint8_t method_status;
    if (read_sock(socket, (char *)&method_status, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Method selection failed\n");
#endif
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    switch (method_status) {
        case STATUS_OK:
            return EXIT_SUCCESS;

        case STATUS_NO_DATA: {
            if (callback) callback->function(RESP_NO_DATA, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }

        case STATUS_METHOD_NOT_IMPLEMENTED:
        case STATUS_UNKNOWN_METHOD: {
            if (callback) callback->function(RESP_METHOD_NOT_ALLOWED, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }

        default: {
            if (callback) callback->function(RESP_SERVER_ERROR, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
    }
}

#if PROTOCOL_MIN <= 1

int version_1(socket_t *socket, uint8_t method, StatusCallback *callback) {
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

    if (method_request(socket, method, callback) != EXIT_SUCCESS) return EXIT_FAILURE;

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket, callback);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket, callback);
        }
        case METHOD_GET_FILE: {
            return get_files_v1(socket, callback);
        }
        case METHOD_SEND_FILE: {
            return send_file_v1(socket, callback);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket, callback);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            if (callback) callback->function(RESP_PROTO_METHOD_ERROR, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
    }
}
#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)

int version_2(socket_t *socket, uint8_t method, StatusCallback *callback) {
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

    if (method_request(socket, method, callback) != EXIT_SUCCESS) return EXIT_FAILURE;

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket, callback);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket, callback);
        }
        case METHOD_GET_FILE: {
            return get_files_v2(socket, callback);
        }
        case METHOD_SEND_FILE: {
            return send_files_v2(socket, callback);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket, callback);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            if (callback) callback->function(RESP_PROTO_METHOD_ERROR, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
    }
}
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)

int version_3(socket_t *socket, uint8_t method, const MethodArgs *args, StatusCallback *callback) {
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

    if (method_request(socket, method, callback) != EXIT_SUCCESS) return EXIT_FAILURE;

    switch (method) {
        case METHOD_GET_TEXT: {
            return get_text_v1(socket, callback);
        }
        case METHOD_SEND_TEXT: {
            return send_text_v1(socket, callback);
        }
        case METHOD_GET_FILE: {
            return get_files_v3(socket, callback);
        }
        case METHOD_SEND_FILE: {
            return send_files_v3(socket, callback);
        }
        case METHOD_GET_IMAGE: {
            return get_image_v1(socket, callback);
        }
        case METHOD_GET_COPIED_IMAGE: {
            return get_copied_image_v3(socket, callback);
        }
        case METHOD_GET_SCREENSHOT: {
            uint16_t display = args->display;
            return get_screenshot_v3(socket, display, callback);
        }
        case METHOD_INFO: {
            return info_v1(socket);
        }
        default: {  // unknown method
            if (callback) callback->function(RESP_PROTO_METHOD_ERROR, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
    }
}
#endif
