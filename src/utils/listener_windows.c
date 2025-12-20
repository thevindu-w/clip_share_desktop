/*
 * utils/listener_windows.c - implementation of clipboard listener for Windows
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

#ifdef _WIN64

#include <stdio.h>
#include <stdlib.h>
#include <utils/clipboard_listener.h>
#include <utils/utils.h>
#include <windows.h>

static volatile int running = 0;
static volatile HINSTANCE instance = NULL;
static volatile HWND hWnd = NULL;
static ListenerCallback clip_callback = NULL;
static volatile WINBOOL listening = FALSE;

static LRESULT CALLBACK ClipWindProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            listening = AddClipboardFormatListener(window);
            return listening ? 0 : -1;
        }
        case WM_DESTROY: {
            return 0;
        }
        case WM_CLIPBOARDUPDATE: {
            if (check_and_delete_temp_file()) {  // send text only if it's not from clip-share
                return 0;
            }
            int copied_type = get_copied_type();
            if (copied_type != COPIED_TYPE_NONE) {
                clip_callback(copied_type);
            }
            return 0;
        }
        default:
            break;
    }
    return DefWindowProc(window, msg, wParam, lParam);
}

int clipboard_listen(ListenerCallback callback) {
    clip_callback = callback;
    instance = GetModuleHandle(NULL);
    char CLASSNAME[] = "clipListen";
    WNDCLASS wc = {.lpfnWndProc = (WNDPROC)ClipWindProc, .hInstance = instance, .lpszClassName = CLASSNAME};
    RegisterClass(&wc);
    hWnd = CreateWindowEx(0, CLASSNAME, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, instance, NULL);
    if (!hWnd) {
#ifdef DEBUG_MODE
        puts("Window creation failed for listener");
#endif
        return EXIT_FAILURE;
    }

    MSG msg;
    running = 1;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return EXIT_SUCCESS;
}

void cleanup_listener(void) {
    running = 0;
    if (hWnd) {
        if (listening) {
            RemoveClipboardFormatListener(hWnd);
            listening = FALSE;
        }
        DestroyWindow(hWnd);
    }
    if (instance) {
        CloseHandle(instance);
        instance = NULL;
    }
}

#endif
