/*
 * clients/cli_client.h - header for CLI client methods
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

#ifndef CLIENTS_CLIENT_H_
#define CLIENTS_CLIENT_H_

#include <stdint.h>

extern void get_text(uint32_t server_addr);

extern void send_text(uint32_t server_addr);

extern void get_files(uint32_t server_addr);

extern void send_files(uint32_t server_addr);

extern void get_image(uint32_t server_addr);

extern void get_copied_image(uint32_t server_addr);

extern void get_screenshot(uint32_t server_addr);
#endif  // CLIENTS_CLIENT_H_
