/*
 * clients/status_cb.h - header for declaring status callback
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

#ifndef CLIENTS_STATUS_CB_H_
#define CLIENTS_STATUS_CB_H_

#include <microhttpd.h>
#include <stddef.h>
#include <stdint.h>

#define RESP_OK MHD_HTTP_OK                                   // Method action is successfully completed
#define RESP_NO_DATA MHD_HTTP_NOT_FOUND                       // Requested data is not available on the server
#define RESP_DATA_ERROR MHD_HTTP_BAD_GATEWAY                  // Received data is not acceptable
#define RESP_METHOD_NOT_ALLOWED MHD_HTTP_FORBIDDEN            // Server doesn't allow this method
#define RESP_PROTO_METHOD_ERROR MHD_HTTP_BAD_REQUEST          // Method code is not valid for the server
#define RESP_PROTO_VERSION_MISMATCH MHD_HTTP_BAD_GATEWAY      // Could not agree on a protocol version with the server
#define RESP_SERVER_ERROR MHD_HTTP_BAD_GATEWAY                // Server response is invalid
#define RESP_COMMUNICATION_FAILURE MHD_HTTP_GATEWAY_TIMEOUT   // Error occurred while communicating with the server
#define RESP_CONNECTION_FAILURE MHD_HTTP_SERVICE_UNAVAILABLE  // Connecting to the server failed
#define RESP_INVALID_ADDRESS MHD_HTTP_BAD_REQUEST             // Server address (format) is invalid
#define RESP_LOCAL_ERROR MHD_HTTP_INTERNAL_SERVER_ERROR       // Desktop client (bridge) ran into an error

typedef struct _status_callback_params {
    int8_t called;
    struct MHD_Connection *connection;
} status_callback_params;

typedef void (*StatusCallbackFn)(unsigned int, const char *, size_t, status_callback_params *);

typedef struct _StatusCallback {
    StatusCallbackFn function;
    status_callback_params *params;
} StatusCallback;

#endif  // CLIENTS_STATUS_CB_H_
