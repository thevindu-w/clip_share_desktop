/*
 * clients/common.c - methods common for all clients
 * Copyright (C) 2026 H. Thevindu J. Wijesekera
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

#include <clients/common.h>
#include <proto/selector.h>
#include <stdlib.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>

void get_server_names(list2 *servers, char sep) {
    if (!servers) {
        return;
    }
    for (unsigned int i = 0; i < servers->len; i++) {
        char *server = servers->array[i];
        socket_t sock;
        uint32_t server_addr;
        if (ipv4_aton(server, &server_addr) != EXIT_SUCCESS) {
            continue;
        }
        connect_server(&sock, server_addr);
        if (IS_NULL_SOCK(sock.type)) {
            continue;
        }
        char *data = NULL;
        MethodArgs args = {0};
        args.data_p = &data;
        int ret = handle_proto(&sock, METHOD_INFO, &args, NULL);
        close_socket_no_wait(&sock);
        if (ret != EXIT_SUCCESS) {
            if (data) {
                free(data);
            }
            continue;
        }
        char *line = strstr(data, "\nserver_name=");
        if (!line) {
            if (data) {
                free(data);
            }
            continue;
        }
        line += 13;
        char *eol = strchr(line, '\n');
        if (eol) {
            *eol = '\0';
        }
        size_t len = strnlen(line, 512);
        memmove(data, line, len);
        data[len] = 0;
        size_t addr_len = strnlen(server, 128);
        size_t tot_len = addr_len + len + 2;
        char *out = realloc(server, tot_len);
        if (!out) {
            if (data) {
                free(data);
            }
            continue;
        }
        snprintf(out + addr_len, tot_len, "%c%s", sep, data);
        free(data);
        servers->array[i] = out;
    }
}
