/*
 * utils/listener_linux.c - implementation of clipboard listener for Linux
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

#ifdef __linux__

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmu/Atoms.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utils/clipboard_listener.h>
#include <utils/utils.h>
#include <xclip/xclip.h>

#ifdef DEBUG_MODE
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

static volatile int running;

int clipboard_listen(ListenerCallback callback) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
#ifdef DEBUG_MODE
        fputs("Connect X server failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }

    Atom clip = XInternAtom(dpy, "CLIPBOARD", 0);
    Window win = DefaultRootWindow(dpy);
    XFixesSelectSelectionInput(dpy, win, clip, XFixesSetSelectionOwnerNotifyMask);

    running = 1;
    while (running) {
        XEvent evt;
        XNextEvent(dpy, &evt);
        if (!running) {
            break;
        }
        if (check_and_delete_temp_file()) {  // send text only if it's not from clip-share
            continue;
        }
        int copied_type = get_copied_type();
        if (copied_type != COPIED_TYPE_NONE) {
            callback(copied_type);
        }
    }
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}

void cleanup_listener(void) { running = 0; }

#endif
