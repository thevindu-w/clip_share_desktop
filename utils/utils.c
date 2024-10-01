/*
 * utils/utils.c - platform specific implementation for utils
 * Copyright (C) 2022-2024 H. Thevindu J. Wijesekera
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

#include <globals.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utils/utils.h>
#ifdef __linux__
#include <X11/Xmu/Atoms.h>
#include <xclip/xclip.h>
#endif
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
static inline char hex2char(char h);
static int url_decode(char *);
#elif defined(_WIN32)
static int utf8_to_wchar_str(const char *utf8str, wchar_t **wstr_p, int *wlen_p);
static inline void _wappend(list2 *lst, const wchar_t *wstr);
#endif

int snprintf_check(char *dest, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(dest, size, fmt, ap);
    va_end(ap);
    return (ret < 0 || ret > (long)size);
}

void error(const char *msg) {
    if (!error_log_file) return;
#ifdef DEBUG_MODE
    fprintf(stderr, "%s\n", msg);
#endif
    FILE *f = open_file(error_log_file, "a");
    // retry with delays if failed
    for (unsigned int i = 0; i < 4; i++) {
        struct timespec interval = {.tv_sec = 0, .tv_nsec = (long)(1 + i * 50) * 1000000L};
        if (f != NULL || nanosleep(&interval, NULL)) break;
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
    exit_wrapper(EXIT_FAILURE);
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

void exit_wrapper(int code) {
    if (error_log_file) free(error_log_file);
    if (cwd) free(cwd);
    clear_config(&configuration);
#ifdef __linux__
    freeAtomPtr(_XA_CLIPBOARD);
    freeAtomPtr(_XA_UTF8_STRING);
#endif
    exit(code);
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
    fseek(fp, 0L, SEEK_END);
    int64_t file_size = ftell(fp);
    rewind(fp);
    return file_size;
}

int is_directory(const char *path, int follow_symlinks) {
    if (path[0] == 0) return 0;  // empty path
    struct stat sb;
    int stat_result;
#if defined(__linux__) || defined(__APPLE__)
    if (follow_symlinks) {
        stat_result = stat(path, &sb);
    } else {
        stat_result = lstat(path, &sb);
    }
#elif defined(_WIN32)
    (void)follow_symlinks;
    wchar_t *wpath;
    if (utf8_to_wchar_str(path, &wpath, NULL) != EXIT_SUCCESS) return -1;
    stat_result = wstat(wpath, &sb);
    free(wpath);
#endif
    if (stat_result == 0) {
        if (S_ISDIR(sb.st_mode)) {
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

/*
 * Allocate the required capacity for the string with EOL=CRLF including the terminating '\0'.
 * Assign the realloced string to *str_p and the length after conversion to *len_p without the terminating '\0'.
 * Returns 0 if all \n are preceded by \r (i.e. no conversion needed).
 * Returns 1 if conversion is needed and it will increase the length.
 * Returns -1 if realloc failed.
 */
static inline int _realloc_for_crlf(char **str_p, uint64_t *len_p) {
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
    uint64_t req_len = ind + increase;
    char *re_str = realloc(str, req_len + 1);  // +1 for terminating '\0'
    if (!re_str) {
        free(str);
        error("realloc failed");
        return -1;
    }
    *str_p = re_str;
    *len_p = req_len;
    return 1;
}

static inline void _convert_to_crlf(char *str, uint64_t new_len) {
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
    int crlf;
#if defined(__linux__) || defined(__APPLE__)
    crlf = 0;
#elif defined(_WIN32)
    crlf = 1;
#endif
    if (force_lf) crlf = 0;
    // realloc if available capacity is not enough
    if (crlf) {
        uint64_t new_len;
        int status = _realloc_for_crlf(str_p, &new_len);
        if (status == 0) return (int64_t)new_len;  // no conversion needed
        if (status < 0 || !*str_p) return -1;      // realloc failed
        _convert_to_crlf(*str_p, new_len);
        return (int64_t)new_len;
    }
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

    list2 *lst = init_list(file_cnt);
    if (!lst) {
        free(fnames);
        return NULL;
    }
    char *fname = file_path;
    for (size_t i = 0; i < file_cnt; i++) {
        size_t off = strnlen(fname, 2047) + 1;
        if (url_decode(fname) == EXIT_FAILURE) break;

        struct stat statbuf;
        if (stat(fname, &statbuf)) {
            fname += off;
            continue;
        }
        if (!S_ISREG(statbuf.st_mode)) {
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

    size_t file_cnt = DragQueryFile(hDrop, (UINT)(-1), NULL, MAX_PATH);

    if (file_cnt <= 0) {
        GlobalUnlock(hGlobal);
        CloseClipboard();
        return NULL;
    }
    list2 *lst = init_list(file_cnt);
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

#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

/*
 * Try to create the directory at path.
 * If the path points to an existing directory or a new directory was created successfuly, returns EXIT_SUCCESS.
 * If the path points to an existing file which is not a directory or creating a directory failed, returns
 * EXIT_FAILURE.
 */
static int _mkdir_check(const char *path);

static int _mkdir_check(const char *path) {
    if (file_exists(path)) {
        if (!is_directory(path, 0)) return EXIT_FAILURE;
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
        if (is_directory(dir_path, 0))
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

#endif  // (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

#if defined(__linux__) || defined(__APPLE__)

static inline char hex2char(char h) {
    if ('0' <= h && h <= '9') return (char)((int)h - '0');
    if ('A' <= h && h <= 'F') return (char)((int)h - 'A' + 10);
    if ('a' <= h && h <= 'f') return (char)((int)h - 'a' + 10);
    return -1;
}

static int url_decode(char *str) {
    if (strncmp("file://", str, 7)) return EXIT_FAILURE;
    char *ptr1 = str;
    const char *ptr2 = str + 7;
    if (!ptr2) return EXIT_FAILURE;
    do {
        char c;
        if (*ptr2 == '%') {
            ptr2++;
            char tmp = *ptr2;
            char c1 = hex2char(tmp);
            if (c1 < 0) return EXIT_FAILURE;  // invalid url
            c = (char)(c1 << 4);
            ptr2++;
            tmp = *ptr2;
            c1 = hex2char(tmp);
            if (c1 < 0) return EXIT_FAILURE;  // invalid url
            c |= c1;
        } else {
            c = *ptr2;
        }
        *ptr1 = c;
        ptr1++;
        ptr2++;
    } while (*ptr2);
    *ptr1 = 0;
    return EXIT_SUCCESS;
}
#endif

#ifdef __linux__

int get_clipboard_text(char **buf_ptr, size_t *len_ptr) {
    if (xclip_util(XCLIP_OUT, NULL, len_ptr, buf_ptr) != EXIT_SUCCESS || *len_ptr <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read text failed. len = %zu\n", *len_ptr);
#endif
        if (*buf_ptr) free(*buf_ptr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    if (fork() > 0) return EXIT_SUCCESS;  // prevent caller from hanging
    if (xclip_util(XCLIP_IN, NULL, &len, &data) != EXIT_SUCCESS) {
        if (data) free(data);
        error_exit("Failed to write to clipboard");
    }
    if (data) free(data);
    exit_wrapper(EXIT_SUCCESS);
}

char *get_copied_files_as_str(int *offset) {
    const char *const expected_target = "x-special/gnome-copied-files";
    char *targets;
    size_t targets_len;
    if (xclip_util(XCLIP_OUT, "TARGETS", &targets_len, &targets) || targets_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read TARGETS. len = %zu\n", targets_len);
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
    if (!found) {
#ifdef DEBUG_MODE
        puts("No copied files");
#endif
        free(targets);
        return NULL;
    }
    free(targets);

    char *fnames;
    size_t fname_len;
    if (xclip_util(XCLIP_OUT, expected_target, &fname_len, &fnames) || fname_len <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read copied files. len = %zu\n", fname_len);
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

static int utf8_to_wchar_str(const char *utf8str, wchar_t **wstr_p, int *wlen_p) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, NULL, 0);
    if (wlen <= 0) return EXIT_FAILURE;
    wchar_t *wstr = malloc((size_t)wlen * sizeof(wchar_t));
    if (!wstr) return EXIT_FAILURE;
    MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, wstr, wlen);
    wstr[wlen - 1] = 0;
    *wstr_p = wstr;
    if (wlen_p) *wlen_p = wlen - 1;
    return EXIT_SUCCESS;
}

int wchar_to_utf8_str(const wchar_t *wstr, char **utf8str_p, int *len_p) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return EXIT_FAILURE;
    char *str = malloc((size_t)len);
    if (!str) return EXIT_FAILURE;
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    str[len - 1] = 0;
    *utf8str_p = str;
    if (len_p) *len_p = len - 1;
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
    int alloc_len;
    if (wchar_to_utf8_str(wcwd, &utf8path, &alloc_len) != EXIT_SUCCESS) {
        free(wcwd);
        return NULL;
    }
    free(wcwd);
    if (alloc_len < len) utf8path = realloc(utf8path, (size_t)len);
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

int get_clipboard_text(char **bufptr, size_t *lenptr) {
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        CloseClipboard();
        return EXIT_FAILURE;
    }
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    char *data;
    int len;
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
    *lenptr = (size_t)len;
    data[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    if (!OpenClipboard(0)) return EXIT_FAILURE;
    wchar_t *wstr;
    int wlen;
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
    EmptyClipboard();
    HANDLE res = SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return (res == NULL ? EXIT_FAILURE : EXIT_SUCCESS);
}
#endif
