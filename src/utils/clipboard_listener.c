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
#include <stdio.h>
#include <utils/clipboard_listener.h>

#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

static void send_to_servers(void) {
    list2 *servers = udp_scan();
    if (!servers) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Scan failed\n");
#endif
        return;
    }
    if (servers->len == 0) {
#ifdef DEBUG_MODE
        fprintf(stderr, "No servers found\n");
#endif
        free_list(servers);
        return;
    }
    for (size_t i = 0; i < servers->len; i++) {
        uint32_t server_addr;
        if (ipv4_aton(servers->array[i], &server_addr) != EXIT_SUCCESS) {
            continue;
        }
        socket_t sock;
        connect_server(server_addr, configuration.app_port, &sock);
        if (IS_NULL_SOCK(sock.type)) {
            return;
        }
        handle_proto(&sock, METHOD_SEND_TEXT, NULL, NULL);
        close_socket_no_wait(&sock);
    }
    free_list(servers);
}

int start_clipboard_listener(void) { return clipboard_listen(&send_to_servers); }
