/*
 * utils/background_sync.h - header for background sync functions
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

#ifndef UTILS_BACKGROUND_SYNC_H_
#define UTILS_BACKGROUND_SYNC_H_

typedef void (*ListenerCallback)(int);

extern void send_to_servers(int type, int8_t is_auto_send);

extern int start_clipboard_listener(void);

extern int clipboard_listen(ListenerCallback callback);

extern void cleanup_listener(void);

#endif  // UTILS_BACKGROUND_SYNC_H_
