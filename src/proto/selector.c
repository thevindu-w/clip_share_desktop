/*
 * proto/selector.c - protocol version selector
 * Copyright (C) 2024-2026 H. Thevindu J. Wijesekera
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

#include <globals.h>
#include <proto/selector.h>
#include <proto/versions.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

// protocol version status
#define PROTOCOL_SUPPORTED 1
#define PROTOCOL_OBSOLETE 2
#define PROTOCOL_UNKNOWN 3

static inline int proto_handler(socket_t *socket, uint8_t version, uint8_t method, const MethodArgs *args,
                                StatusCallback *callback) {
    MethodArgs methodArgs = {0};
    if (!args) {
        args = &methodArgs;
    }
    switch (version) {
#if PROTOCOL_MIN <= 1
        case 1: {
            return version_1(socket, method, args, callback);
        }
#endif
#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
        case 2: {
            return version_2(socket, method, args, callback);
        }
#endif
#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
        case 3: {
            return version_3(socket, method, args, callback);
        }
#endif
        default: {  // invalid or unknown version
            error("Invalid protocol version");
            if (callback) callback->function(RESP_PROTO_VERSION_MISMATCH, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
    }
}

static inline int negotiate_unknown_proto(socket_t *socket, uint16_t min_version, uint16_t max_version,
                                          uint8_t *version_p, uint8_t *status_p, StatusCallback *callback) {
    if (read_sock(socket, (char *)version_p, 1) != EXIT_SUCCESS) {  // Get offer
#ifdef DEBUG_MODE
        fprintf(stderr, "negotiate protocol version failed\n");
#endif
        if (callback) {
            callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        }
        return EXIT_FAILURE;
    }
    if (*version_p < min_version || *version_p > max_version) {  // Reject offer
        *status_p = 0;
        write_sock(socket, (char *)status_p, 1);
        error("Protocol version negotiation failed");
        if (callback) {
            callback->function(RESP_PROTO_VERSION_MISMATCH, NULL, 0, callback->params);
        }
        return EXIT_FAILURE;
    }
    if (write_sock(socket, (char *)version_p, 1) != EXIT_SUCCESS) {  // Accept offer
#ifdef DEBUG_MODE
        fprintf(stderr, "negotiate protocol version failed\n");
#endif
        if (callback) {
            callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_proto(socket_t *socket, uint8_t method, const MethodArgs *args, StatusCallback *callback) {
    const uint16_t min_version = configuration.min_proto_version;
    const uint16_t max_version = configuration.max_proto_version;
    uint8_t version = (uint8_t)max_version;
    uint8_t status;

    if (write_sock(socket, (char *)&version, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send protocol version failed\n");
#endif
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    if (read_sock(socket, (char *)&status, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "read protocol version status failed\n");
#endif
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    switch (status) {
        case PROTOCOL_SUPPORTED: {  // protocol version accepted
            break;
        }

        case PROTOCOL_OBSOLETE: {
            error("Client's protocol versions are obsolete");
            if (callback) callback->function(RESP_PROTO_VERSION_MISMATCH, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }

        case PROTOCOL_UNKNOWN: {
            if (negotiate_unknown_proto(socket, min_version, max_version, &version, &status, callback) !=
                EXIT_SUCCESS) {
                return EXIT_FAILURE;
            }
            break;
        }

        default: {
            error("Server sent invalid protocol version status");
            if (callback) callback->function(RESP_SERVER_ERROR, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
    }

    return proto_handler(socket, version, method, args, callback);
}
