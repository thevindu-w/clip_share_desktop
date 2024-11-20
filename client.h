/*
 * client.h - header for client methods
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

#ifndef CLIENT_H_
#define CLIENT_H_

extern void get_text(void);

extern void send_text(void);

extern void get_files(void);

extern void send_files(void);

extern void get_image(void);

extern void get_copied_image(void);

extern void get_screenshot(void);
#endif  // CLIENT_H_
