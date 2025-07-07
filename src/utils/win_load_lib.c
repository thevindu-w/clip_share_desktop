/*
 * utils/win_load_lib.c - load dynamic libraries at runtime on Windows
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

// clang-format off
#include <windows.h>
// clang-format on
#include <shellapi.h>
#include <stdio.h>
#include <utils/win_load_lib.h>

extern FARPROC __imp_GetUserProfileDirectoryW;

extern FARPROC __imp_Shell_NotifyIconA;
extern FARPROC __imp_DragQueryFileW;

extern FARPROC __imp_OpenProcessToken;

static HMODULE module_userenv = NULL;
static HMODULE module_shell32 = NULL;
static HMODULE module_advapi32 = NULL;

FARPROC __imp_GetUserProfileDirectoryW = NULL;
FARPROC __imp_Shell_NotifyIconA = NULL;
FARPROC __imp_DragQueryFileW = NULL;
FARPROC __imp_OpenProcessToken;

static inline HMODULE LoadLibWrapper(const char *dll) {
    HMODULE module = LoadLibraryA(dll);
    if (!module) {
        fprintf(stderr, "Failed to load DLL %s\n", dll);
        return NULL;
    }
    return module;
}

static inline FARPROC GetProcAddressWrapper(HMODULE module, const char *funcName) {
    FARPROC func = GetProcAddress(module, funcName);
    if (!func) {
        fprintf(stderr, "Failed to load function %s\n", funcName);
        return NULL;
    }
    return func;
}

#pragma GCC diagnostic ignored "-Wcast-function-type"
static int _load_libs(void) {
    module_userenv = LoadLibWrapper("USERENV.DLL");
    if (!module_userenv) return EXIT_FAILURE;

    __imp_GetUserProfileDirectoryW = GetProcAddressWrapper(module_userenv, "GetUserProfileDirectoryW");
    if (!__imp_GetUserProfileDirectoryW) return EXIT_FAILURE;

    module_shell32 = LoadLibWrapper("SHELL32.DLL");
    if (!module_shell32) return EXIT_FAILURE;

    __imp_Shell_NotifyIconA = GetProcAddressWrapper(module_shell32, "Shell_NotifyIconA");
    if (!__imp_Shell_NotifyIconA) return EXIT_FAILURE;

    __imp_DragQueryFileW = GetProcAddressWrapper(module_shell32, "DragQueryFileW");
    if (!__imp_DragQueryFileW) return EXIT_FAILURE;

    module_advapi32 = LoadLibWrapper("ADVAPI32.DLL");
    if (!module_advapi32) return EXIT_FAILURE;

    __imp_OpenProcessToken = GetProcAddressWrapper(module_advapi32, "OpenProcessToken");
    if (!__imp_OpenProcessToken) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

int load_libs(void) {
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (_load_libs() != EXIT_SUCCESS) {
        cleanup_libs();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static inline void clean_lib(HMODULE *module_p) {
    if (*module_p) {
        FreeLibrary(*module_p);
        *module_p = NULL;
    }
}

void cleanup_libs(void) {
    clean_lib(&module_userenv);
    clean_lib(&module_shell32);
    clean_lib(&module_advapi32);
}

#endif
