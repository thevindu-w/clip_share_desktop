/*
 * utils/listener_macos.c - implementation of clipboard listener for macOS
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

#ifdef __APPLE__

#include <stdlib.h>
#include <utils/clipboard_listener.h>
#include <utils/utils.h>

int clipboard_listen(ListenerCallback callback) {
    (void)callback;
    error("Clipboard listener is not implemented on macOS");
    return EXIT_FAILURE;
}

void cleanup_listener(void) {}

#endif
