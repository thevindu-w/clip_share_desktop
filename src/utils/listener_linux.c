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

static int check_text(void) {
    char *targets;
    uint32_t targets_len;
    if (xclip_util(XCLIP_OUT, "TARGETS", &targets_len, &targets) || targets_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read TARGETS. len = %" PRIu32 "\n", targets_len);
#endif
        if (targets) {
            free(targets);
        }
        return EXIT_FAILURE;
    }
    char found = 0;
    char *copy = targets;
    const char *token;
    while ((token = strsep(&copy, "\n"))) {
        if (!strncmp(token, "text/plain", 10) || !strcmp(token, "UTF8_STRING")) {
            found = 1;
        }
        if (!strncmp(token, "x-special/gnome-copied-files", 28)) {
            found = 0;
            break;
        }
    }
    free(targets);
    if (!found) {
#ifdef DEBUG_MODE
        puts("No copied text");
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

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
        if (check_text() == EXIT_SUCCESS) {
            callback();
        }
    }
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}

void cleanup_listener(void) { running = 0; }

#endif
