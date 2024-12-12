/*
 * clients/status_cb.h - header for declaring status callback
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

#ifndef CLIENTS_STATUS_CB_H_
#define CLIENTS_STATUS_CB_H_

#include <microhttpd.h>
#include <stddef.h>
#include <stdint.h>

#define OK MHD_HTTP_OK
#define NO_DATA MHD_HTTP_NOT_FOUND
#define PROTO_METHOD_ERROR MHD_HTTP_BAD_GATEWAY
#define PROTOCOL_VERSION_MISMATCH MHD_HTTP_BAD_GATEWAY
#define DATA_ERROR MHD_HTTP_BAD_GATEWAY
#define COMMUNICATION_FAILURE MHD_HTTP_GATEWAY_TIMEOUT
#define CONNECTION_FAILURE MHD_HTTP_SERVICE_UNAVAILABLE
#define INVALID_ADDRESS MHD_HTTP_BAD_REQUEST
#define LOCAL_ERROR MHD_HTTP_INTERNAL_SERVER_ERROR

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
