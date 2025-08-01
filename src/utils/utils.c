/*
 * utils/utils.c - platform specific implementation for utils
 * Copyright (C) 2022-2025 H. Thevindu J. Wijesekera
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

#define _FILE_OFFSET_BITS 64

#ifdef DEBUG_MODE
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#include <dirent.h>
#include <fcntl.h>
#include <globals.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utils/clipboard_listener.h>
#include <utils/utils.h>
#ifdef __linux__
#include <X11/Xmu/Atoms.h>
#include <xclip/xclip.h>
#endif
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#ifdef _WIN64
#include <utils/win_load_lib.h>
#endif
#endif

#define MAX_RECURSE_DEPTH 256

#if defined(__linux__) || defined(__APPLE__)

#define TEMP_FILE "/tmp/clipshare-copied"

static inline int8_t hex2char(char h);
static int url_decode(char *, uint32_t *len_p);
#elif defined(_WIN32)
static wchar_t *temp_file = NULL;

static int utf8_to_wchar_str(const char *utf8str, wchar_t **wstr_p, uint32_t *wlen_p);
static inline void _wappend(list2 *lst, const wchar_t *wstr);
#endif
static inline int milli_sleep(unsigned int millis);

void print_usage(const char *prog_name) {
    fprintf(stderr, "\nUsage: %s [OPTION]\n", prog_name);
    fprintf(stderr, "  or:  %s -c COMMAND <server-address-ipv4> [optional args]\n", prog_name);
    fprintf(stderr,
            "Options available:\n"
            "\t-h : Display this help list\n"
            "\t-s : Stop running instances\n"
            "\t-v : Print version\n"
            "\t-d : Daemonize (default) - Run the web client in background and exit the main process\n"
            "\t-D : No-Daemonize - Run the web client in the main process\n"
            "\t-c COMMAND [server-address-ipv4]: Run a CLI command. This needs a command\n");
    fprintf(stderr,
            "Commands available:\n"
            "\tsc : Scan - server address is not needed\n"
            "\tg  : Get copied text\n"
            "\ts  : Send copied text\n"
            "\tfg : Get copied files\n"
            "\tfs : Send copied files\n"
            "\ti  : Get image\n"
            "\tic : Get copied image\n"
            "\tis : Get screenshot - Display number can be used as an optional arg.\n");
    fprintf(stderr,
            "\nExample: %s -c g 192.168.21.42\n"
            "\tThis command gets copied text from the device having IP address 192.168.21.42\n"
            "\nExample: %s -c is 192.168.21.42 1\n"
            "\tThis command gets a screenshot of screen number 1 from the device having IP address 192.168.21.42\n"
            "\nExample: %s -c sc\n"
            "\tThis command scans and outputs the IP addresses of available servers.\n"
            "\nExample: %s -h\n"
            "\tThis command scans and outputs the IP addresses of available servers.\n\n",
            prog_name, prog_name, prog_name, prog_name);
}

int snprintf_check(char *dest, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(dest, size, fmt, ap);
    va_end(ap);
    return (ret < 0 || ret > (long)size);
}

#ifdef _WIN32
static inline int milli_sleep(unsigned int millis) {
    Sleep(millis);
    return 0;
}
#else
static inline int milli_sleep(unsigned int millis) {
    struct timespec interval = {.tv_sec = 0, .tv_nsec = (long)millis * 1000000L};
    return nanosleep(&interval, NULL);
}
#endif

void error(const char *msg) {
    if (!error_log_file) return;
#ifdef DEBUG_MODE
    fprintf(stderr, "%s\n", msg);
#endif
    FILE *f = open_file(error_log_file, "a");
    // retry with delays if failed
    for (unsigned int i = 0; i < 4; i++) {
        unsigned int interval = 1 + i * 50;
        if (f != NULL || milli_sleep(interval)) break;
        f = open_file(error_log_file, "a");
    }
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
#if defined(__linux__) || defined(__APPLE__)
        chmod(error_log_file, S_IWUSR | S_IWGRP | S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
#endif
    }
}

void error_exit(const char *msg) {
    error(msg);
    exit(EXIT_FAILURE);
}

#ifdef __linux__
typedef struct _DisplayRec {
    struct _DisplayRec *next;
    Display *dpy;
    Atom atom;
} DisplayRec;

struct _AtomRec {
    _Xconst char *name;
    DisplayRec *head;
};

static void freeAtomPtr(AtomPtr atomPtr) {
    if (atomPtr) {
        DisplayRec *next = atomPtr->head;
        atomPtr->head = NULL;
        while (next) {
            DisplayRec *tmp = next->next;
            XtFree((char *)(next));
            next = tmp;
        }
    }
}
#endif

void cleanup(void) {
#ifdef DEBUG_MODE
    puts("Cleaning up resources before exit");
#endif
    cleanup_listener();
    if (error_log_file) {
        free(error_log_file);
        error_log_file = NULL;
    }
    if (cwd) {
        free(cwd);
        cwd = NULL;
    }
    clear_config(&configuration);
#ifdef __linux__
    if (_XA_CLIPBOARD) freeAtomPtr(_XA_CLIPBOARD);
    if (_XA_UTF8_STRING) freeAtomPtr(_XA_UTF8_STRING);
#elif defined(_WIN32)
    WSACleanup();
#ifdef _WIN64
    if (temp_file) {
        free(temp_file);
        temp_file = NULL;
    }
    cleanup_libs();
#endif
#endif
}

int file_exists(const char *file_name) {
    if (file_name[0] == 0) return 0;  // empty path
    int f_ok;
#if defined(__linux__) || defined(__APPLE__)
    f_ok = access(file_name, F_OK);
#elif defined(_WIN32)
    wchar_t *wfname;
    if (utf8_to_wchar_str(file_name, &wfname, NULL) != EXIT_SUCCESS) return -1;
    f_ok = _waccess(wfname, F_OK);
    free(wfname);
#endif
    return f_ok == 0;
}

void *realloc_or_free(void *ptr, size_t size) {
    void *tmp = realloc(ptr, size);
    if (!tmp) {
        if (ptr) free(ptr);
        error("realloc failed");
    }
    return tmp;
}

int64_t get_file_size(FILE *fp) {
    struct stat statbuf;
    if (fstat(fileno(fp), &statbuf)) {
#ifdef DEBUG_MODE
        puts("fstat failed");
#endif
        return -1;
    }
    if (!S_ISREG(statbuf.st_mode)) {
#ifdef DEBUG_MODE
        puts("not a file");
#endif
        return -1;
    }
    fseeko(fp, 0L, SEEK_END);
    int64_t file_size = ftello(fp);
    rewind(fp);
    return file_size;
}

int is_directory(const char *path, int follow_symlinks) {
    if (path[0] == 0) return -1;  // empty path
    int stat_result;
#if defined(__linux__) || defined(__APPLE__)
    struct stat sb;
    if (follow_symlinks) {
        stat_result = stat(path, &sb);
    } else {
        stat_result = lstat(path, &sb);
    }
#elif defined(_WIN32)
    (void)follow_symlinks;
    struct _stat64 sb;
    wchar_t *wpath;
    if (utf8_to_wchar_str(path, &wpath, NULL) != EXIT_SUCCESS) return -1;
    stat_result = _wstat64(wpath, &sb);
    free(wpath);
#endif
    if (stat_result == 0) {
        if (S_ISDIR(sb.st_mode)) {
            return 1;
        } else {
            return 0;
        }
    }
    return -1;
}

#ifdef _WIN32
/*
 * Allocate the required capacity for the string with EOL=CRLF including the terminating '\0'.
 * Assign the realloced string to *str_p and the length after conversion to *len_p without the terminating '\0'.
 * Returns 0 if all LF are preceded by CR (i.e. no conversion needed).
 * Returns 1 if conversion is needed and it will increase the length.
 * Returns -1 if realloc failed, and free() the *str_p.
 */
static inline int _realloc_for_crlf(char **str_p, size_t *len_p) {
    char *str = *str_p;
    size_t increase = 0;
    size_t ind;
    for (ind = 0; str[ind]; ind++) {
        if (str[ind] == '\n' && (ind == 0 || str[ind - 1] != '\r')) {
            // needs increasing
            increase++;
        }
    }
    if (!increase) {
        *len_p = ind;
        return 0;
    }
    uint64_t req_len = (uint64_t)ind + increase;
    if (req_len >= 0xFFFFFFFFUL) {
        free(str);
        error("realloc size too large");
        return -1;
    }
    *str_p = realloc_or_free(str, (size_t)req_len + 1);  // +1 for terminating '\0'
    if (!*str_p) return -1;
    *len_p = (size_t)req_len;
    return 1;
}

static inline void _convert_to_crlf(char *str, size_t new_len) {
    // converting to CRLF expands string. Therefore, start from the end to avoid overwriting
    size_t new_ind = new_len - 1;
    str[new_len] = 0;  // terminating '\0'
    for (size_t cur_ind = strnlen(str, new_len + 1) - 1;; cur_ind--) {
        char c = str[cur_ind];
        str[new_ind--] = c;
        if (c == '\n' && (cur_ind == 0 || str[cur_ind - 1] != '\r')) {
            // add the missing \r before \n
            str[new_ind--] = '\r';
        }
        if (cur_ind == 0) break;
    }
}
#endif

static inline int64_t _convert_to_lf(char *str) {
    // converting to CRLF shrinks string. Therefore, start from the begining to avoid overwriting
    const char *old_ptr = str;
    char *new_ptr = str;
    char c;
    while ((c = *old_ptr)) {
        old_ptr++;
        if (c != '\r' || *old_ptr != '\n') {
            *new_ptr = c;
            new_ptr++;
        }
    }
    *new_ptr = '\0';
    return new_ptr - str;
}

int64_t convert_eol(char **str_p, int force_lf) {
#ifdef _WIN32
    if (!force_lf) {  // convert to CRLF
        size_t new_len;
        // realloc if available capacity is not enough
        int status = _realloc_for_crlf(str_p, &new_len);
        if (status == 0) {  // no conversion needed
            return (int64_t)new_len;
        }
        if (status < 0 || !*str_p) {  // realloc failed
            return -1;
        }
        _convert_to_crlf(*str_p, new_len);
        return (int64_t)new_len;
    }
#else
    (void)force_lf;
#endif
    return _convert_to_lf(*str_p);
}

#if PROTOCOL_MIN <= 1

#if defined(__linux__) || defined(__APPLE__)

list2 *get_copied_files(void) {
    int offset = 0;
    char *fnames = get_copied_files_as_str(&offset);
    if (!fnames) {
        return NULL;
    }
    char *file_path = fnames + offset;

    size_t file_cnt = 1;
    for (char *ptr = file_path; *ptr; ptr++) {
        if (*ptr == '\n') {
            file_cnt++;
            *ptr = 0;
        }
    }
    if (file_cnt >= 0xFFFFFFFFUL) {
        free(fnames);
        return NULL;
    }

    list2 *lst = init_list((uint32_t)file_cnt);
    if (!lst) {
        free(fnames);
        return NULL;
    }
    char *fname = file_path;
    for (size_t i = 0; i < file_cnt; i++) {
        size_t off = strnlen(fname, 2047) + 1;
        if (url_decode(fname, NULL) != EXIT_SUCCESS) break;

        struct stat statbuf;
        if (stat(fname, &statbuf)) {
#ifdef DEBUG_MODE
            puts("stat failed");
#endif
            fname += off;
            continue;
        }
        if (!S_ISREG(statbuf.st_mode)) {
#ifdef DEBUG_MODE
            printf("not a file : %s\n", fname);
#endif
            fname += off;
            continue;
        }
        append(lst, strdup(fname));
        fname += off;
    }
    free(fnames);
    return lst;
}

#elif defined(_WIN32)

list2 *get_copied_files(void) {
    if (!OpenClipboard(0)) return NULL;
    if (!IsClipboardFormatAvailable(CF_HDROP)) {
        CloseClipboard();
        return NULL;
    }
    HGLOBAL hGlobal = (HGLOBAL)GetClipboardData(CF_HDROP);
    if (!hGlobal) {
        CloseClipboard();
        return NULL;
    }
    HDROP hDrop = (HDROP)GlobalLock(hGlobal);
    if (!hDrop) {
        CloseClipboard();
        return NULL;
    }

    size_t file_cnt = DragQueryFileW(hDrop, (UINT)(-1), NULL, MAX_PATH);

    if (file_cnt <= 0 || file_cnt >= 0xFFFFFFFFUL) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return NULL;
    }
    list2 *lst = init_list((uint32_t)file_cnt);
    if (!lst) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return NULL;
    }

    wchar_t fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++) {
        fileName[0] = '\0';
        DragQueryFileW(hDrop, (UINT)i, fileName, MAX_PATH);
        DWORD attr = GetFileAttributesW(fileName);
        DWORD dontWant =
            FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
        if (attr & dontWant) {
#ifdef DEBUG_MODE
            wprintf(L"not a file : %s\n", fileName);
#endif
            continue;
        }
        _wappend(lst, fileName);
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
    return lst;
}

#endif

#endif  // PROTOCOL_MIN <= 1

/*
 * Try to create the directory at path.
 * If the path points to an existing directory or a new directory was created successfuly, returns EXIT_SUCCESS.
 * If the path points to an existing file which is not a directory or creating a directory failed, returns
 * EXIT_FAILURE.
 */
static int _mkdir_check(const char *path) {
    if (file_exists(path)) {
        if (is_directory(path, 0) != 1) return EXIT_FAILURE;
    } else {
        int status;  // success=0 and failure=non-zero
#if defined(__linux__) || defined(__APPLE__)
        status = mkdir(path, S_IRWXU | S_IRWXG);
#elif defined(_WIN32)
        wchar_t *wpath;
        if (utf8_to_wchar_str(path, &wpath, NULL) == EXIT_SUCCESS) {
            status = CreateDirectoryW(wpath, NULL) != TRUE;
            free(wpath);
        } else {
            status = 1;
        }
#endif
        if (status) {
#ifdef DEBUG_MODE
            printf("Error creating directory %s\n", path);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int mkdirs(const char *dir_path) {
    if (dir_path[0] != '.') return EXIT_FAILURE;  // path must be relative and start with .

    if (file_exists(dir_path)) {
        if (is_directory(dir_path, 0) == 1)
            return EXIT_SUCCESS;
        else
            return EXIT_FAILURE;
    }

    size_t len = strnlen(dir_path, 2050);
    if (len > 2048) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char path[len + 1];
    strncpy(path, dir_path, sizeof(path));
    if (path[sizeof(path) - 1] != 0) {
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i <= len; i++) {
        if (path[i] != PATH_SEP && path[i] != 0) continue;
        path[i] = 0;
        if (_mkdir_check(path) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
        if (i < len) path[i] = PATH_SEP;
    }
    return EXIT_SUCCESS;
}

list2 *list_dir(const char *dirname) {
#if defined(__linux__) || defined(__APPLE__)
    DIR *d = opendir(dirname);
#elif defined(_WIN32)
    wchar_t *wdname;
    if (utf8_to_wchar_str(dirname, &wdname, NULL) != EXIT_SUCCESS) {
        return NULL;
    }
    _WDIR *d = _wopendir(wdname);
    free(wdname);
#endif
    if (!d) {
#ifdef DEBUG_MODE
        puts("Error opening directory");
#endif
        return NULL;
    }
    list2 *lst = init_list(2);
    if (!lst) return NULL;
    while (1) {
#if defined(__linux__) || defined(__APPLE__)
        const struct dirent *dir = readdir(d);
        const char *filename;
#elif defined(_WIN32)
        const struct _wdirent *dir = _wreaddir(d);
        const wchar_t *filename;
#endif
        if (dir == NULL) break;
        filename = dir->d_name;
#if defined(__linux__) || defined(__APPLE__)
        if (!(strcmp(filename, ".") && strcmp(filename, ".."))) continue;
        append(lst, strdup(filename));
#elif defined(_WIN32)
        if (!(wcscmp(filename, L".") && wcscmp(filename, L".."))) continue;
        _wappend(lst, filename);
#endif
    }
#if defined(__linux__) || defined(__APPLE__)
    (void)closedir(d);
#elif defined(_WIN32)
    (void)_wclosedir(d);
#endif
    return lst;
}

#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

#if defined(__linux__) || defined(__APPLE__)

/*
 * Check if the path is a file or a directory.
 * If the path is a directory, calls _recurse_dir() on that.
 * Otherwise, appends the path to the list
 */
static void _process_path(const char *path, list2 *lst, int depth, int include_leaf_dirs);

/*
 * Recursively append all file paths in the directory and its subdirectories
 * to the list.
 * maximum recursion depth is limited to MAX_RECURSE_DEPTH
 */
static void _recurse_dir(const char *_path, list2 *lst, int depth, int include_leaf_dirs);

static void _process_path(const char *path, list2 *lst, int depth, int include_leaf_dirs) {
    struct stat sb;
    if (lstat(path, &sb) != 0) return;
    if (S_ISDIR(sb.st_mode)) {
        _recurse_dir(path, lst, depth + 1, include_leaf_dirs);
    } else if (S_ISREG(sb.st_mode)) {
        append(lst, strdup(path));
    }
}

static void _recurse_dir(const char *_path, list2 *lst, int depth, int include_leaf_dirs) {
    if (depth > MAX_RECURSE_DEPTH) return;
    DIR *d = opendir(_path);
    if (!d) {
#ifdef DEBUG_MODE
        printf("Error opening directory %s", _path);
#endif
        return;
    }
    size_t p_len = strnlen(_path, 2050);
    if (p_len > 2048) {
        error("Too long file name.");
        (void)closedir(d);
        return;
    }
    char path[p_len + 2];
    strncpy(path, _path, p_len + 1);
    path[p_len + 1] = 0;
    if (path[p_len - 1] != PATH_SEP) {
        path[p_len++] = PATH_SEP;
        path[p_len] = '\0';
    }
    const struct dirent *dir;
    int is_empty = 1;
    while ((dir = readdir(d)) != NULL) {
        const char *filename = dir->d_name;
        if (!(strcmp(filename, ".") && strcmp(filename, ".."))) continue;
        is_empty = 0;
        const size_t _fname_len = strnlen(filename, sizeof(dir->d_name));
        if (_fname_len + p_len > 2048) {
            error("Too long file name.");
            (void)closedir(d);
            return;
        }
        char pathname[_fname_len + p_len + 1];
        strncpy(pathname, path, p_len);
        strncpy(pathname + p_len, filename, _fname_len + 1);
        pathname[p_len + _fname_len] = 0;
        _process_path(pathname, lst, depth, include_leaf_dirs);
    }
    if (include_leaf_dirs && is_empty) {
        append(lst, strdup(path));
    }
    (void)closedir(d);
}

void get_copied_dirs_files(dir_files *dfiles_p, int include_leaf_dirs) {
    dfiles_p->lst = NULL;
    dfiles_p->path_len = 0;
    int offset = 0;
    char *fnames = get_copied_files_as_str(&offset);
    if (!fnames) {
        return;
    }
    char *file_path = fnames + offset;

    size_t file_cnt = 1;
    for (char *ptr = file_path; *ptr; ptr++) {
        if (*ptr == '\n') {
            file_cnt++;
            *ptr = 0;
        }
    }
    if (file_cnt >= 0xFFFFFFFFUL) {
        free(fnames);
        return;
    }

    list2 *lst = init_list((uint32_t)file_cnt);
    if (!lst) {
        free(fnames);
        return;
    }
    dfiles_p->lst = lst;
    char *fname = file_path;
    for (size_t i = 0; i < file_cnt; i++) {
        const size_t off = strnlen(fname, 2047) + 1;
        uint32_t fname_len;
        if (url_decode(fname, &fname_len) != EXIT_SUCCESS || fname_len == 0 || fname_len > 2047) break;
        // fname has changed after url_decode
        if (i == 0) {
            if (fname[fname_len - 1] == PATH_SEP) fname[fname_len - 1] = 0;  // if directory, remove ending /
            const char *sep_ptr = strrchr(fname, PATH_SEP);
            if (sep_ptr > fname) {
                dfiles_p->path_len = (size_t)sep_ptr - (size_t)fname + 1;
            }
        }

        struct stat statbuf;
        if (stat(fname, &statbuf)) {
#ifdef DEBUG_MODE
            puts("stat failed");
#endif
            fname += off;
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            _recurse_dir(fname, lst, 1, include_leaf_dirs);
            fname += off;
        } else if (S_ISREG(statbuf.st_mode)) {
            append(lst, strdup(fname));
            fname += off;
        }
    }
    free(fnames);
}

#elif defined(_WIN32)

/*
 * Check if the path is a file or a directory.
 * If the path is a directory, calls _recurse_dir() on that.
 * Otherwise, appends the path to the list
 */
static void _process_path(const wchar_t *path, list2 *lst, int depth, int include_leaf_dirs);

/*
 * Recursively append all file paths in the directory and its subdirectories
 * to the list.
 * maximum recursion depth is limited to MAX_RECURSE_DEPTH
 */
static void _recurse_dir(const wchar_t *_path, list2 *lst, int depth, int include_leaf_dirs);

static void _process_path(const wchar_t *path, list2 *lst, int depth, int include_leaf_dirs) {
    struct _stat64 sb;
    if (_wstat64(path, &sb) != 0) return;
    if (S_ISDIR(sb.st_mode)) {
        _recurse_dir(path, lst, depth + 1, include_leaf_dirs);
    } else if (S_ISREG(sb.st_mode)) {
        _wappend(lst, path);
    }
}

static void _recurse_dir(const wchar_t *_path, list2 *lst, int depth, int include_leaf_dirs) {
    if (depth > MAX_RECURSE_DEPTH) return;
    _WDIR *d = _wopendir(_path);
    if (!d) {
#ifdef DEBUG_MODE
        wprintf(L"Error opening directory %s", _path);
#endif
        return;
    }
    size_t p_len = wcsnlen(_path, 2050);
    if (p_len > 2048) {
        error("Too long file name.");
        (void)_wclosedir(d);
        return;
    }
    wchar_t path[p_len + 2];
    wcsncpy(path, _path, p_len + 1);
    path[p_len + 1] = 0;
    if (path[p_len - 1] != PATH_SEP) {
        path[p_len++] = PATH_SEP;
        path[p_len] = '\0';
    }
    const struct _wdirent *dir;
    int is_empty = 1;
    while ((dir = _wreaddir(d)) != NULL) {
        const wchar_t *filename = dir->d_name;
        if (!(wcscmp(filename, L".") && wcscmp(filename, L".."))) continue;
        is_empty = 0;
        const size_t _fname_len = wcslen(filename);
        if (_fname_len + p_len > 2048) {
            error("Too long file name.");
            (void)_wclosedir(d);
            return;
        }
        wchar_t pathname[_fname_len + p_len + 1];
        wcsncpy(pathname, path, p_len);
        wcsncpy(pathname + p_len, filename, _fname_len + 1);
        pathname[p_len + _fname_len] = 0;
        _process_path(pathname, lst, depth, include_leaf_dirs);
    }
    if (include_leaf_dirs && is_empty) {
        _wappend(lst, wcsdup(path));
    }
    (void)_wclosedir(d);
}

void get_copied_dirs_files(dir_files *dfiles_p, int include_leaf_dirs) {
    dfiles_p->lst = NULL;
    dfiles_p->path_len = 0;

    if (!OpenClipboard(0)) return;
    if (!IsClipboardFormatAvailable(CF_HDROP)) {
        CloseClipboard();
        return;
    }
    HGLOBAL hGlobal = (HGLOBAL)GetClipboardData(CF_HDROP);
    if (!hGlobal) {
        CloseClipboard();
        return;
    }
    HDROP hDrop = (HDROP)GlobalLock(hGlobal);
    if (!hDrop) {
        CloseClipboard();
        return;
    }

    size_t file_cnt = DragQueryFileW(hDrop, (UINT)(-1), NULL, MAX_PATH);

    if (file_cnt <= 0 || file_cnt >= 0xFFFFFFFFUL) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return;
    }
    list2 *lst = init_list((uint32_t)file_cnt);
    if (!lst) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return;
    }
    dfiles_p->lst = lst;
    wchar_t fileName[MAX_PATH + 1];
    for (size_t i = 0; i < file_cnt; i++) {
        fileName[0] = 0;
        DragQueryFileW(hDrop, (UINT)i, fileName, MAX_PATH);
        DWORD attr = GetFileAttributesW(fileName);
        DWORD dontWant = FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
        if (attr & dontWant) {
#ifdef DEBUG_MODE
            wprintf(L"not a file or dir : %s\n", fileName);
#endif
            continue;
        }
        if (i == 0) {
            wchar_t *sep_ptr = wcsrchr(fileName, PATH_SEP);
            if (sep_ptr > fileName) {
                dfiles_p->path_len = (size_t)(sep_ptr - fileName + 1);
            }
        }
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            _recurse_dir(fileName, lst, 1, include_leaf_dirs);
        } else {  // regular file
            _wappend(lst, fileName);
        }
    }
    GlobalUnlock(hGlobal);
    CloseClipboard();
}

#endif

#endif  // (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

#if defined(__linux__) || defined(__APPLE__)

void create_temp_file(void) {
    int fd = open(TEMP_FILE, O_CREAT, 0666);
    close(fd);
}

int check_and_delete_temp_file(void) {
    int exists = file_exists(TEMP_FILE);
    if (!exists) {
        return 0;
    }
    remove(TEMP_FILE);
    return 1;
}

static inline int8_t hex2char(char h) {
    if ('0' <= h && h <= '9') return (int8_t)((int)h - '0');
    if ('A' <= h && h <= 'F') return (int8_t)((int)h - 'A' + 10);
    if ('a' <= h && h <= 'f') return (int8_t)((int)h - 'a' + 10);
    return -1;
}

static int url_decode(char *str, uint32_t *len_p) {
    if (strncmp("file://", str, 7)) return EXIT_FAILURE;
    char *ptr1 = str;
    const char *ptr2 = str + 7;
    if (!ptr2) return EXIT_FAILURE;
    do {
        char c;
        if (*ptr2 == '%') {
            ptr2++;
            char tmp = *ptr2;
            int8_t c1 = hex2char(tmp);
            if (c1 < 0) return EXIT_FAILURE;  // invalid url
            c = (char)(c1 << 4);
            ptr2++;
            tmp = *ptr2;
            c1 = hex2char(tmp);
            if (c1 < 0) return EXIT_FAILURE;  // invalid url
#if defined(__CHAR_UNSIGNED__) && __CHAR_UNSIGNED__
            c |= (char)c1;
#else
            c |= c1;
#endif
        } else {
            c = *ptr2;
        }
        *ptr1 = c;
        ptr1++;
        ptr2++;
    } while (*ptr2);
    *ptr1 = 0;
    if (len_p) *len_p = (uint32_t)(ptr1 - str);
    return EXIT_SUCCESS;
}
#endif

#ifdef __linux__

int get_clipboard_text(char **buf_ptr, uint32_t *len_ptr) {
    if (xclip_util(XCLIP_OUT, NULL, len_ptr, buf_ptr) != EXIT_SUCCESS || *len_ptr <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read text failed. len = %" PRIu32 "\n", *len_ptr);
#endif
        if (*buf_ptr) free(*buf_ptr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, uint32_t len) {
    if (fork() > 0) return EXIT_SUCCESS;  // prevent caller from hanging
    create_temp_file();
    if (xclip_util(XCLIP_IN, NULL, &len, &data) != EXIT_SUCCESS) {
        if (data) free(data);
        error_exit("Failed to write to clipboard");
    }
    if (data) free(data);
    exit(EXIT_SUCCESS);
}

char *get_copied_files_as_str(int *offset) {
    const char *const expected_target = "x-special/gnome-copied-files";
    char *targets;
    uint32_t targets_len;
    if (xclip_util(XCLIP_OUT, "TARGETS", &targets_len, &targets) || targets_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read TARGETS. len = %" PRIu32 "\n", targets_len);
#endif
        if (targets) free(targets);
        return NULL;
    }
    char found = 0;
    char *copy = targets;
    const char *token;
    while ((token = strsep(&copy, "\n"))) {
        if (!strcmp(token, expected_target)) {
            found = 1;
            break;
        }
    }
    free(targets);
    if (!found) {
#ifdef DEBUG_MODE
        puts("No copied files");
#endif
        return NULL;
    }

    char *fnames;
    uint32_t fname_len;
    if (xclip_util(XCLIP_OUT, expected_target, &fname_len, &fnames) || fname_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read copied files. len = %" PRIu32 "\n", fname_len);
#endif
        if (fnames) free(fnames);
        return NULL;
    }
    fnames[fname_len] = 0;

    char *file_path = strchr(fnames, '\n');
    if (!file_path) {
        free(fnames);
        return NULL;
    }
    *file_path = 0;
    if (strcmp(fnames, "copy") && strcmp(fnames, "cut")) {
        free(fnames);
        return NULL;
    }
    *offset = (int)(file_path - fnames) + 1;
    return fnames;
}

#elif defined(_WIN32)
static int set_temp_file(void) {
    if (!temp_file) {
        const char *tmp_dir = getenv("TEMP");
        if (!tmp_dir) {
#ifdef DEBUG_MODE
            puts("getting TEMP failed");
#endif
            return EXIT_FAILURE;
        }
        char tmp_path[1024];
        if (snprintf_check(tmp_path, sizeof(tmp_path), "%s%c%s", tmp_dir, PATH_SEP, "clipshare-copied")) {
#ifdef DEBUG_MODE
            puts("TEMP too long");
#endif
            return EXIT_FAILURE;
        }
        tmp_path[1023] = 0;
        if ((utf8_to_wchar_str(tmp_path, &temp_file, NULL) != EXIT_SUCCESS) || !temp_file) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void create_temp_file(void) {
    if (set_temp_file() != EXIT_SUCCESS) {
        return;
    }
    int fd = _wopen(temp_file, O_CREAT, _S_IWRITE);
    close(fd);
}

int check_and_delete_temp_file(void) {
    if (set_temp_file() != EXIT_SUCCESS) {
        return 0;
    }
    int f_ok = _waccess(temp_file, F_OK);
    if (f_ok != 0) {
        return 0;
    }
    _wremove(temp_file);
    return 1;
}

int rename_file(const char *old_name, const char *new_name) {
    wchar_t *wold;
    wchar_t *wnew;
    if (utf8_to_wchar_str(old_name, &wold, NULL) != EXIT_SUCCESS) return -1;
    if (utf8_to_wchar_str(new_name, &wnew, NULL) != EXIT_SUCCESS) {
        free(wold);
        return -1;
    }
    int result = _wrename(wold, wnew);
    free(wold);
    free(wnew);
    return result;
}

int remove_directory(const char *path) {
    wchar_t *wpath;
    if (utf8_to_wchar_str(path, &wpath, NULL) != EXIT_SUCCESS) return -1;
    int result = (RemoveDirectoryW(wpath) == FALSE);
    free(wpath);
    return result;
}

static int utf8_to_wchar_str(const char *utf8str, wchar_t **wstr_p, uint32_t *wlen_p) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, NULL, 0);
    if (wlen <= 0) return EXIT_FAILURE;
    wchar_t *wstr = malloc((size_t)wlen * sizeof(wchar_t));
    if (!wstr) return EXIT_FAILURE;
    MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, wstr, wlen);
    wstr[wlen - 1] = 0;
    *wstr_p = wstr;
    if (wlen_p) *wlen_p = (uint32_t)(wlen - 1);
    return EXIT_SUCCESS;
}

int wchar_to_utf8_str(const wchar_t *wstr, char **utf8str_p, uint32_t *len_p) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return EXIT_FAILURE;
    char *str = malloc((size_t)len);
    if (!str) return EXIT_FAILURE;
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    str[len - 1] = 0;
    *utf8str_p = str;
    if (len_p) *len_p = (uint32_t)(len - 1);
    return EXIT_SUCCESS;
}

int chdir_wrapper(const char *path) {
    wchar_t *wpath;
    if (utf8_to_wchar_str(path, &wpath, NULL) != EXIT_SUCCESS) return -1;
    int result = _wchdir(wpath);
    free(wpath);
    return result;
}

char *getcwd_wrapper(int len) {
    wchar_t *wcwd = _wgetcwd(NULL, len);
    if (!wcwd) return NULL;
    char *utf8path;
    uint32_t alloc_len;
    if (wchar_to_utf8_str(wcwd, &utf8path, &alloc_len) != EXIT_SUCCESS) {
        free(wcwd);
        return NULL;
    }
    free(wcwd);
    if ((int)alloc_len < len) utf8path = realloc_or_free(utf8path, (size_t)len);
    return utf8path;
}

FILE *open_file(const char *filename, const char *mode) {
    wchar_t *wfname;
    wchar_t *wmode;
    if (utf8_to_wchar_str(filename, &wfname, NULL) != EXIT_SUCCESS) return NULL;
    if (utf8_to_wchar_str(mode, &wmode, NULL) != EXIT_SUCCESS) {
        free(wfname);
        return NULL;
    }
    FILE *fp = _wfopen(wfname, wmode);
    free(wfname);
    free(wmode);
    return fp;
}

int remove_file(const char *filename) {
    wchar_t *wfname;
    if (utf8_to_wchar_str(filename, &wfname, NULL) != EXIT_SUCCESS) return -1;
    int result = _wremove(wfname);
    free(wfname);
    return result;
}

/*
 * A wrapper to append() for wide strings.
 * Convert wchar_t * string to utf-8 and append to list
 */
static inline void _wappend(list2 *lst, const wchar_t *wstr) {
    char *utf8path;
    if (wchar_to_utf8_str(wstr, &utf8path, NULL) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        wprintf(L"Error while converting file path: %s\n", wstr);
#endif
        return;
    }
    append(lst, utf8path);
}

int get_clipboard_text(char **bufptr, uint32_t *lenptr) {
    if (!OpenClipboard(0)) {
        Sleep(10);  // retry after a short delay
        if (!OpenClipboard(0)) {
            return EXIT_FAILURE;
        }
    }
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        CloseClipboard();
        return EXIT_FAILURE;
    }
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    char *data;
    uint32_t len;
    if (wchar_to_utf8_str((wchar_t *)h, &data, &len) != EXIT_SUCCESS) {
        data = NULL;
        len = 0;
    }
    CloseClipboard();

    if (!data) {
#ifdef DEBUG_MODE
        fputs("clipboard data is null\n", stderr);
#endif
        *lenptr = 0;
        return EXIT_FAILURE;
    }
    *bufptr = data;
    *lenptr = (uint32_t)len;
    data[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, uint32_t len) {
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    wchar_t *wstr;
    uint32_t wlen;
    char prev = data[len];
    data[len] = 0;
    if (utf8_to_wchar_str(data, &wstr, &wlen) != EXIT_SUCCESS) {
        data[len] = prev;
        CloseClipboard();
        return EXIT_FAILURE;
    }
    data[len] = prev;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (size_t)(wlen + 1) * sizeof(wchar_t));
    wcscpy_s(GlobalLock(hMem), (rsize_t)(wlen + 1), wstr);
    GlobalUnlock(hMem);
    free(wstr);
    create_temp_file();
    EmptyClipboard();
    HANDLE res = SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return (res == NULL ? EXIT_FAILURE : EXIT_SUCCESS);
}
#endif
