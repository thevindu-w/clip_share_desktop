/*
 * proto/methods.c - platform independent implementation of methods
 * Copyright (C) 2024-2025 H. Thevindu J. Wijesekera
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

#define __STDC_FORMAT_MACROS
#include <globals.h>
#include <inttypes.h>
#include <proto/methods.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#ifndef __GLIBC__
#define __GLIBC__ 0
#endif
#ifndef __NO_INLINE__
#define __NO_INLINE__
#endif
#include <unistr.h>

#define FILE_BUF_SZ 65536L  // 64 KiB
#define MAX_FILE_NAME_LENGTH 2048

#define MIN(x, y) (x < y ? x : y)

#define STR1(z) #z
#define STR(z) STR1(z)
#define JOIN(a, b, c) STR(a) b STR(c)
const char *bad_path = JOIN(PATH_SEP, "..", PATH_SEP);

/*
 * Send a data buffer to the peer.
 * Sends the length first and then the data buffer.
 */
static inline int _send_data(socket_t *socket, int64_t length, const char *data) {
    if (send_size(socket, length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send length failed\n");
#endif
        return EXIT_FAILURE;
    }
    if (length < 0 || write_sock(socket, data, (uint64_t)length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send data failed\n");
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/*
 * Common function to send files.
 */
static int _send_files_common(int version, socket_t *socket, list2 *file_list, size_t path_len,
                              StatusCallback *callback);

/*
 * Common function to send files.
 */
static int _get_files_dirs(int version, socket_t *socket, StatusCallback *callback);

/*
 * Common function to save files.
 */
static int _save_file_common(int version, socket_t *socket, const char *file_name, StatusCallback *callback);

/*
 * Check if the file name is valid.
 * A file name is valid only if it's a valid UTF-8 non-empty string, and contains no invalid characters \x00 to \x1f.
 * returns EXIT_SUCCESS if the file name is valid.
 * Otherwise, returns EXIT_FAILURE.
 */
static inline int _is_valid_fname(const char *fname, size_t name_length);

static int _transfer_single_file(int version, socket_t *socket, const char *file_path, size_t path_len,
                                 StatusCallback *callback);

int send_text_v1(socket_t *socket, StatusCallback *callback) {
    uint32_t length = 0;
    char *buf = NULL;
    if (get_clipboard_text(&buf, &length) != EXIT_SUCCESS || length <= 0 ||
        length > configuration.max_text_length) {  // do not change the order
#ifdef DEBUG_MODE
        printf("clipboard read text failed. len = %" PRIu32 "\n", length);
#endif
        if (buf) free(buf);
        if (callback) callback->function(RESP_NO_DATA, NULL, 0, callback->params);
        return EXIT_SUCCESS;
    }
#ifdef DEBUG_MODE
    printf("Len = %" PRIu32 "\n", length);
    if (length < 1024) {
        fwrite(buf, 1, length, stdout);
        puts("");
    }
#endif
    int64_t new_len = convert_eol(&buf, 1);
    if (new_len <= 0 || !buf) {
        if (callback) callback->function(RESP_NO_DATA, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (_send_data(socket, new_len, buf) != EXIT_SUCCESS) {
        free(buf);
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (callback) callback->function(RESP_OK, buf, (size_t)new_len, callback->params);
    free(buf);
    close_socket(socket);
    return EXIT_SUCCESS;
}

int get_text_v1(socket_t *socket, StatusCallback *callback) {
    int64_t length;
    if (read_size(socket, &length) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("Len = %" PRIi64 "\n", length);
#endif
    // limit maximum length to max_text_length
    if (length <= 0 || length > configuration.max_text_length) {
        if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    char *data = malloc((size_t)length + 1);
    if (!data || read_sock(socket, data, (uint64_t)length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Read data failed\n", stderr);
#endif
        if (data) free(data);
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    data[length] = 0;
    if (u8_check((uint8_t *)data, (size_t)length)) {
#ifdef DEBUG_MODE
        fputs("Invalid UTF-8\n", stderr);
#endif
        free(data);
        if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    if (length < 1024) puts(data);
#endif
    if (callback) callback->function(RESP_OK, data, (size_t)length, callback->params);
    length = convert_eol(&data, 0);
    if (length <= 0 || !data) return EXIT_FAILURE;
    close_socket_no_wait(socket);
    put_clipboard_text(data, (uint32_t)length);
    free(data);
    return EXIT_SUCCESS;
}

static int _transfer_regular_file(socket_t *socket, const char *file_path, const char *filename, size_t fname_len,
                                  StatusCallback *callback) {
    FILE *fp = open_file(file_path, "rb");
    if (!fp) {
        error("Couldn't open some files");
        return EXIT_FAILURE;
    }
    int64_t file_size = get_file_size(fp);
    if (file_size < 0 || file_size > configuration.max_file_size) {
#ifdef DEBUG_MODE
        printf("file size = %" PRIi64 "\n", file_size);
#endif
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (_send_data(socket, (int64_t)fname_len, filename) != EXIT_SUCCESS) {
        fclose(fp);
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    if (send_size(socket, file_size) != EXIT_SUCCESS) {
        fclose(fp);
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    char data[FILE_BUF_SZ];
    while (file_size > 0) {
        size_t read = fread(data, 1, FILE_BUF_SZ, fp);
        if (read == 0) continue;
        if (write_sock(socket, data, read) != EXIT_SUCCESS) {
            fclose(fp);
            if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
        file_size -= (ssize_t)read;
    }
    fclose(fp);
    return EXIT_SUCCESS;
}

#if PROTOCOL_MAX >= 3
static int _transfer_directory(socket_t *socket, const char *filename, size_t fname_len, StatusCallback *callback) {
    if (_send_data(socket, (int64_t)fname_len, filename) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (send_size(socket, -1) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
#endif

static int _transfer_single_file(int version, socket_t *socket, const char *file_path, size_t path_len,
                                 StatusCallback *callback) {
    const char *tmp_fname;
    switch (version) {
#if PROTOCOL_MIN <= 1
        case 1: {
            tmp_fname = strrchr(file_path, PATH_SEP);
            if (tmp_fname == NULL) {
                tmp_fname = file_path;
            } else {
                tmp_fname++;  // remove '/'
            }
            break;
        }
#endif
#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)
        case 2:
        case 3: {
            tmp_fname = file_path + path_len;
            break;
        }
#else
        (void)path_len;
#endif
        default: {
            return EXIT_FAILURE;
        }
    }

    const size_t fname_len = strnlen(tmp_fname, MAX_FILE_NAME_LENGTH + 1);
    if (fname_len <= 0 || fname_len > MAX_FILE_NAME_LENGTH) {
        error("Invalid file name length.");
        return EXIT_FAILURE;
    }
    char filename[fname_len + 1];
    strncpy(filename, tmp_fname, fname_len);
    filename[fname_len] = 0;

#if (PATH_SEP != '/') && (PROTOCOL_MAX > 1)
    if (version > 1) {
        // path separator is always / when communicating
        for (size_t ind = 0; ind < fname_len; ind++) {
            if (filename[ind] == PATH_SEP) filename[ind] = '/';
        }
    }
#endif

#if PROTOCOL_MAX >= 3
    if (filename[fname_len - 1] == '/') {  // filename is converted to have / as path separator on all platforms
        filename[fname_len - 1] = 0;
        return _transfer_directory(socket, filename, fname_len - 1, callback);
    }
#endif
    return _transfer_regular_file(socket, file_path, filename, fname_len, callback);
}

static int _send_files_common(int version, socket_t *socket, list2 *file_list, size_t path_len,
                              StatusCallback *callback) {
    if (!file_list || file_list->len == 0 || file_list->len >= 0xFFFFFFFFUL) {
        if (callback) callback->function(RESP_NO_DATA, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    uint32_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;
#ifdef DEBUG_MODE
    printf("%" PRIu32 "file(s)\n", file_cnt);
#endif
    if (version > 1 && (send_size(socket, (int64_t)file_cnt) != EXIT_SUCCESS)) {
        free_list(file_list);
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (version == 1) file_cnt = 1;  // proto v1 can only send 1 file

    for (uint32_t i = 0; i < file_cnt; i++) {
        const char *file_path = files[i];
#ifdef DEBUG_MODE
        printf("file name = %s\n", file_path);
#endif

        if (_transfer_single_file(version, socket, file_path, path_len, callback) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            puts("Transfer failed");
#endif
            return EXIT_FAILURE;
        }
    }
    if (callback) callback->function(RESP_OK, NULL, 0, callback->params);
    close_socket(socket);
    return EXIT_SUCCESS;
}

static int _save_file_common(int version, socket_t *socket, const char *file_name, StatusCallback *callback) {
    int64_t file_size;
    if (read_size(socket, &file_size) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("data len = %" PRIi64 "\n", file_size);
#endif
    if (file_size > configuration.max_file_size) {
        if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
    if (file_size == -1 && version == 3) {
        return mkdirs(file_name);
    }
#else
    (void)version;
#endif
    if (file_size < 0) {
        if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    FILE *file = open_file(file_name, "wb");
    if (!file) {
        error("Couldn't create some files");
        return EXIT_FAILURE;
    }

    char data[FILE_BUF_SZ];
    while (file_size) {
        size_t read_len = file_size < FILE_BUF_SZ ? (size_t)file_size : FILE_BUF_SZ;
        if (read_sock(socket, data, read_len) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            puts("recieve error");
#endif
            fclose(file);
            remove_file(file_name);
            if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
            return EXIT_FAILURE;
        }
        if (fwrite(data, 1, read_len, file) < read_len) {
            fclose(file);
            remove_file(file_name);
            return EXIT_FAILURE;
        }
        file_size -= (int64_t)read_len;
    }

    fclose(file);

#ifdef DEBUG_MODE
    printf("file saved : %s\n", file_name);
#endif
    return EXIT_SUCCESS;
}

static inline int _is_valid_fname(const char *fname, size_t name_length) {
    if (u8_check((const uint8_t *)fname, name_length)) {
#ifdef DEBUG_MODE
        fputs("Invalid UTF-8\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    do {
        if ((unsigned char)*fname < 32) return EXIT_FAILURE;
        fname++;
    } while (*fname);
    return EXIT_SUCCESS;
}

static char *get_abs_path(const char *rel_path, size_t rel_path_max_len) {
    char *path = (char *)malloc(cwd_len + rel_path_max_len + 2);  // for PATH_SEP and null terminator
    if (!path) return NULL;
    strncpy(path, cwd, cwd_len);
    size_t p_len = cwd_len;
    path[p_len++] = PATH_SEP;
    strncpy(path + p_len, rel_path, rel_path_max_len);
    path[cwd_len + rel_path_max_len + 1] = 0;  // +1 for PATH_SEP
    return path;
}

#if PROTOCOL_MIN <= 1
/*
 * Get only the base name.
 * Path seperator is assumed to be '/' regardless of the platform.
 */
static inline int _get_base_name(char *file_name, size_t name_length) {
    const char *base_name = strrchr(file_name, '/');  // path separator is / when communicating
    if (base_name) {
        base_name++;                                                         // don't want the '/' before the file name
        memmove(file_name, base_name, strnlen(base_name, name_length) + 1);  // overlapping memory area
    }
    return EXIT_SUCCESS;
}

int send_file_v1(socket_t *socket, StatusCallback *callback) {
    list2 *file_list = get_copied_files();
    int ret = _send_files_common(1, socket, file_list, 0, callback);
    if (file_list) free_list(file_list);
    return ret;
}

int get_files_v1(socket_t *socket, StatusCallback *callback) { return _get_files_dirs(1, socket, callback); }
#endif

static inline int _save_image_common(socket_t *socket, StatusCallback *callback) {
    struct timeval ts;
    gettimeofday(&ts, NULL);
    uint64_t millis = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_usec / 1000L;
    char file_name[40];
    if (snprintf_check(file_name, sizeof(file_name), "%" PRIx64 ".png", millis)) return EXIT_FAILURE;
    int status = _save_file_common(1, socket, file_name, callback);
    if (callback) {
        if (status == EXIT_SUCCESS) {
            callback->function(RESP_OK, file_name, strnlen(file_name, sizeof(file_name) - 1), callback->params);
        } else {
            callback->function(RESP_LOCAL_ERROR, NULL, 0, callback->params);
        }
    }

    if (status != EXIT_SUCCESS) return EXIT_FAILURE;
    list2 *dest_files = init_list(1);
    if (!dest_files) return EXIT_FAILURE;
    char *abs_path = get_abs_path(file_name, 40);
    if (!abs_path) {
        free_list(dest_files);
        return EXIT_FAILURE;
    }
    append(dest_files, abs_path);
    if (set_clipboard_cut_files(dest_files) != EXIT_SUCCESS) status = EXIT_FAILURE;
    free_list(dest_files);
    return status;
}

int get_image_v1(socket_t *socket, StatusCallback *callback) { return _save_image_common(socket, callback); }

int info_v1(socket_t *socket) {
    // TODO(thevindu-w): implement
    (void)socket;
    return EXIT_SUCCESS;
}

/*
 * Make parent directories for path
 */
static inline int _make_directories(const char *path) {
    char *base_name = strrchr(path, PATH_SEP);
    if (!base_name) return EXIT_FAILURE;
    *base_name = 0;
    if (mkdirs(path) != EXIT_SUCCESS) {
        *base_name = PATH_SEP;
        return EXIT_FAILURE;
    }
    *base_name = PATH_SEP;
    return EXIT_SUCCESS;
}

static int save_file(int version, socket_t *socket, const char *dirname, StatusCallback *callback) {
    int64_t fname_size;
    if (read_size(socket, &fname_size) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("name_len = %" PRIi64 "\n", fname_size);
#endif
    // limit file name length to MAX_FILE_NAME_LENGTH chars
    if (fname_size <= 0 || fname_size > MAX_FILE_NAME_LENGTH) {
        if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    const size_t name_length = (size_t)fname_size;
    char file_name[name_length + 1];
    if (read_sock(socket, file_name, name_length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Read file name failed\n", stderr);
#endif
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }

    file_name[name_length] = 0;
    if (_is_valid_fname(file_name, name_length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        printf("Invalid filename \'%s\'\n", file_name);
#endif
        if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (file_name[name_length - 1] == '/') file_name[name_length - 1] = 0;  // remove trailing /

#if PATH_SEP != '/'
    // replace '/' with PATH_SEP
    for (size_t ind = 0; ind < name_length; ind++) {
        if (file_name[ind] == '/') {
            file_name[ind] = PATH_SEP;
            if (ind > 0 && file_name[ind - 1] == PATH_SEP) {  // "//" in path not allowed
                if (callback) callback->function(RESP_DATA_ERROR, NULL, 0, callback->params);
                return EXIT_FAILURE;
            }
        }
    }
#endif

#if PROTOCOL_MIN <= 1
    if (version == 1) {
        if (_get_base_name(file_name, (size_t)name_length) != EXIT_SUCCESS) return EXIT_FAILURE;

        // PATH_SEP is not allowed in version 1 get files
        if (strchr(file_name, PATH_SEP)) return EXIT_FAILURE;  // all '/'s are converted to PATH_SEP
    }
#endif

    char new_path[name_length + 20];
    if (file_name[0] == PATH_SEP) {
        if (snprintf_check(new_path, name_length + 20, "%s%s", dirname, file_name)) return EXIT_FAILURE;
    } else {
        if (snprintf_check(new_path, name_length + 20, "%s%c%s", dirname, PATH_SEP, file_name)) return EXIT_FAILURE;
    }

    // path must not contain /../ (go to parent dir)
    if (strstr(new_path, bad_path)) return EXIT_FAILURE;

    // make parent directories
    if (version > 1 && _make_directories(new_path) != EXIT_SUCCESS) return EXIT_FAILURE;

    // check if file exists
    if (file_exists(new_path)) return EXIT_FAILURE;

    return _save_file_common(version, socket, new_path, callback);
}

static char *_check_and_rename(const char *filename, const char *dirname) {
    const size_t name_len = strnlen(filename, MAX_FILE_NAME_LENGTH + 1);
    if (name_len > MAX_FILE_NAME_LENGTH) {
        error("Too long file name.");
        return NULL;
    }
    const size_t name_max_len = name_len + 20;
    char old_path[name_max_len];
    if (snprintf_check(old_path, name_max_len, "%s%c%s", dirname, PATH_SEP, filename)) return NULL;

    char new_path[name_max_len];
    if (configuration.working_dir != NULL || strcmp(filename, CONFIG_FILE)) {
        // "./" is important to prevent file names like "C:\path"
        if (snprintf_check(new_path, name_max_len, ".%c%s", PATH_SEP, filename)) return NULL;
    } else {
        // do not create file named clipshare-desktop.conf. "./" is important to prevent file names like "C:\path"
        if (snprintf_check(new_path, name_max_len, ".%c1_%s", PATH_SEP, filename)) return NULL;
    }

    // if new_path already exists, use a different file name
    int n = 1;
    while (file_exists(new_path)) {
        if (n > 999999L) return NULL;
        if (snprintf_check(new_path, name_max_len, ".%c%i_%s", PATH_SEP, n++, filename)) return NULL;
    }

    if (rename_file(old_path, new_path)) {
#ifdef DEBUG_MODE
        printf("Rename failed : %s\n", new_path);
#endif
        return NULL;
    }

    char *path = get_abs_path(new_path + 2, name_max_len);  // +2 for ./ (new_path always starts with ./)
    return path;
}

static int _get_files_dirs(int version, socket_t *socket, StatusCallback *callback) {
    int64_t cnt;
    if (read_size(socket, &cnt) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (cnt <= 0 || cnt >= 0xFFFFFFFFLL) {
        if (callback) callback->function(RESP_NO_DATA, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    char dirname[17];
    unsigned id = (unsigned)time(NULL);
    do {
        if (snprintf_check(dirname, 17, ".%c%x", PATH_SEP, id)) return EXIT_FAILURE;
        id = (unsigned)rand();
    } while (file_exists(dirname));

    if (mkdirs(dirname) != EXIT_SUCCESS) return EXIT_FAILURE;

    for (int64_t file_num = 0; file_num < cnt; file_num++) {
        if (save_file(version, socket, dirname, callback) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }
    close_socket_no_wait(socket);

    list2 *files = list_dir(dirname);
    if (!files) return EXIT_FAILURE;
    int status = EXIT_SUCCESS;
    list2 *dest_files = init_list(files->len);
    if (!dest_files) {
        free_list(files);
        return EXIT_FAILURE;
    }
    for (uint32_t i = 0; i < files->len; i++) {
        const char *filename = files->array[i];
        char *new_path = _check_and_rename(filename, dirname);
        if (!new_path) {
            status = EXIT_FAILURE;
            continue;
        }
        append(dest_files, new_path);
    }
    free_list(files);
    if (status == EXIT_SUCCESS && remove_directory(dirname)) status = EXIT_FAILURE;
    if (callback) callback->function(RESP_OK, NULL, 0, callback->params);
    if (status == EXIT_SUCCESS && set_clipboard_cut_files(dest_files) != EXIT_SUCCESS) status = EXIT_FAILURE;
    free_list(dest_files);
    return status;
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
int send_files_v2(socket_t *socket, StatusCallback *callback) {
    dir_files copied_dir_files;
    get_copied_dirs_files(&copied_dir_files, 0);
    int ret = _send_files_common(2, socket, copied_dir_files.lst, copied_dir_files.path_len, callback);
    if (copied_dir_files.lst) free_list(copied_dir_files.lst);
    return ret;
}

int get_files_v2(socket_t *socket, StatusCallback *callback) { return _get_files_dirs(2, socket, callback); }
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
int get_copied_image_v3(socket_t *socket, StatusCallback *callback) { return _save_image_common(socket, callback); }

int get_screenshot_v3(socket_t *socket, uint16_t display, StatusCallback *callback) {
    if (send_size(socket, (int32_t)display) != EXIT_SUCCESS) {
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    unsigned char status;
    if (read_sock(socket, (char *)&status, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Display selection failed\n");
#endif
        if (callback) callback->function(RESP_COMMUNICATION_FAILURE, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    if (status != STATUS_OK) {
        if (callback) callback->function(RESP_NO_DATA, NULL, 0, callback->params);
        return EXIT_FAILURE;
    }
    return _save_image_common(socket, callback);
}

int send_files_v3(socket_t *socket, StatusCallback *callback) {
    dir_files copied_dir_files;
    get_copied_dirs_files(&copied_dir_files, 1);
    int ret = _send_files_common(3, socket, copied_dir_files.lst, copied_dir_files.path_len, callback);
    if (copied_dir_files.lst) free_list(copied_dir_files.lst);
    return ret;
}

int get_files_v3(socket_t *socket, StatusCallback *callback) { return _get_files_dirs(3, socket, callback); }
#endif
