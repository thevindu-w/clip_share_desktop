#include <globals.h>
#include <proto/methods.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define FILE_BUF_SZ 65536  // 64 KiB
#define MAX_FILE_NAME_LENGTH 2048

#define MIN(x, y) (x < y ? x : y)

/*
 * Common function to send files.
 */
static int _send_files_common(int version, sock_t socket, list2 *file_list, size_t path_len);

/*
 * Common function to save files in get_files and get_image methods.
 */
static int _save_file_common(int version, sock_t socket, const char *file_name);

/*
 * Check if the file name is valid.
 * A file name is valid only if it's a valid UTF-8 non-empty string, and contains no invalid characters \x00 to \x1f.
 * returns EXIT_SUCCESS if the file name is valid.
 * Otherwise, returns EXIT_FAILURE.
 */
static inline int _is_valid_fname(const char *fname, size_t name_length);

static int _transfer_single_file(int version, sock_t socket, const char *file_path, size_t path_len);

int get_text_v1(sock_t socket) {
    int64_t length;
    if (read_size(socket, &length) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("Len = %zi\n", (ssize_t)length);
#endif
    // limit maximum length to max_text_length
    if (length <= 0 || length > configuration.max_text_length) return EXIT_FAILURE;

    char *data = malloc((uint64_t)length + 1);
    if (read_sock(socket, data, (uint64_t)length) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
        fputs("Read data failed\n", stderr);
#endif
        free(data);
        return EXIT_FAILURE;
    }
    data[length] = 0;
    if (u8_check((uint8_t *)data, (size_t)length)) {
#ifdef DEBUG_MODE
        fputs("Invalid UTF-8\n", stderr);
#endif
        free(data);
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    if (length < 1024) puts(data);
#endif
    length = convert_eol(&data, 0);
    if (length < 0) return EXIT_FAILURE;
    close_socket(socket);
    put_clipboard_text(data, (size_t)length);
    free(data);
    return EXIT_SUCCESS;
}

int send_text_v1(sock_t socket) {
    size_t length = 0;
    char *buf = NULL;
    if (get_clipboard_text(&buf, &length) != EXIT_SUCCESS || length <= 0 ||
        length > configuration.max_text_length) {  // do not change the order
#ifdef DEBUG_MODE
        printf("clipboard read text failed. len = %zu\n", length);
#endif
        if (buf) free(buf);
        return EXIT_SUCCESS;
    }
    int64_t new_len = convert_eol(&buf, 1);
    if (new_len <= 0) return EXIT_FAILURE;
    if (send_size(socket, new_len) != EXIT_SUCCESS) {
        free(buf);
        return EXIT_FAILURE;
    }
    if (write_sock(socket, buf, (uint64_t)new_len) != EXIT_SUCCESS) {
        free(buf);
        return EXIT_FAILURE;
    }
    free(buf);
    return EXIT_SUCCESS;
}

static int _transfer_regular_file(sock_t socket, const char *file_path, const char *filename, size_t fname_len) {
    FILE *fp = open_file(file_path, "rb");
    if (!fp) {
        error("Couldn't open some files");
        return EXIT_FAILURE;
    }
    int64_t file_size = get_file_size(fp);
    if (file_size < 0 || file_size > configuration.max_file_size) {
#ifdef DEBUG_MODE
        printf("file size = %lli\n", (long long)file_size);
#endif
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (send_size(socket, (int64_t)fname_len) == EXIT_FAILURE) {
        fclose(fp);
        return EXIT_FAILURE;
    }
    if (write_sock(socket, filename, fname_len) == EXIT_FAILURE) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    if (send_size(socket, file_size) == EXIT_FAILURE) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    char data[FILE_BUF_SZ];
    while (file_size > 0) {
        size_t read = fread(data, 1, FILE_BUF_SZ, fp);
        if (read == 0) continue;
        if (write_sock(socket, data, read) == EXIT_FAILURE) {
            fclose(fp);
            return EXIT_FAILURE;
        }
        file_size -= (ssize_t)read;
    }
    fclose(fp);
    return EXIT_SUCCESS;
}

static int _transfer_single_file(int version, sock_t socket, const char *file_path, size_t path_len) {
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

    const size_t _tmp_len = strnlen(tmp_fname, MAX_FILE_NAME_LENGTH);
    if (_tmp_len > MAX_FILE_NAME_LENGTH) {
        error("Too long file name.");
        return EXIT_FAILURE;
    }
    char filename[_tmp_len + 1];
    strncpy(filename, tmp_fname, _tmp_len);
    filename[_tmp_len] = 0;
    const size_t fname_len = strnlen(filename, _tmp_len);
    if (fname_len > MIN(_tmp_len, MAX_FILE_NAME_LENGTH)) {
        return EXIT_FAILURE;
    }

#if PATH_SEP != '/' && PROTOCOL_MAX > 1
    if (version > 1) {
        // path separator is always / when communicating with the client
        for (size_t ind = 0; ind < fname_len; ind++) {
            if (filename[ind] == PATH_SEP) filename[ind] = '/';
        }
    }
#endif

#if PROTOCOL_MAX >= 3
    if (filename[fname_len - 1] == '/') {  // filename is converted to have / as path separator on all platforms
        filename[fname_len - 1] = 0;
        return _transfer_directory(socket, filename, fname_len - 1);
    }
#endif
    return _transfer_regular_file(socket, file_path, filename, fname_len);
}

static int _send_files_common(int version, sock_t socket, list2 *file_list, size_t path_len) {
    if (!file_list || file_list->len == 0) {
        if (file_list) free_list(file_list);
        return EXIT_SUCCESS;
    }

    size_t file_cnt = file_list->len;
    char **files = (char **)file_list->array;

    if (version > 1 && (send_size(socket, (int64_t)file_cnt) != EXIT_SUCCESS)) {
        free_list(file_list);
        return EXIT_FAILURE;
    }
    if (version == 1) file_cnt = 1;  // proto v1 can only send 1 file

    for (size_t i = 0; i < file_cnt; i++) {
        const char *file_path = files[i];
#ifdef DEBUG_MODE
        printf("file name = %s\n", file_path);
#endif

        if (_transfer_single_file(version, socket, file_path, path_len) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            puts("Transfer failed");
#endif
            free_list(file_list);
            return EXIT_FAILURE;
        }
    }
    free_list(file_list);
    return EXIT_SUCCESS;
}

static int _save_file_common(int version, sock_t socket, const char *file_name) {
    int64_t file_size;
    if (read_size(socket, &file_size) != EXIT_SUCCESS) return EXIT_FAILURE;
    if (file_size > configuration.max_file_size) {
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
        if (read_sock(socket, data, read_len) == EXIT_FAILURE) {
#ifdef DEBUG_MODE
            puts("recieve error");
#endif
            fclose(file);
            remove_file(file_name);
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

#if PROTOCOL_MIN <= 1
/*
 * Get only the base name.
 * Path seperator is assumed to be '/' regardless of the platform.
 */
static inline int _get_base_name(char *file_name, size_t name_length) {
    const char *base_name = strrchr(file_name, '/');  // path separator is / when communicating with the client
    if (base_name) {
        base_name++;                                                         // don't want the '/' before the file name
        memmove(file_name, base_name, strnlen(base_name, name_length) + 1);  // overlapping memory area
    }
    return EXIT_SUCCESS;
}

/*
 * Change file name if exists
 */
static inline int _rename_if_exists(char *file_name, size_t max_len) {
    char tmp_fname[max_len + 1];
    if (configuration.working_dir != NULL || strcmp(file_name, "clipshare.conf")) {
        if (snprintf_check(tmp_fname, max_len, ".%c%s", PATH_SEP, file_name)) return EXIT_FAILURE;
    } else {
        // do not create file named clipshare.conf
        if (snprintf_check(tmp_fname, max_len, ".%c1_%s", PATH_SEP, file_name)) return EXIT_FAILURE;
    }
    int n = 1;
    while (file_exists(tmp_fname)) {
        if (n > 999999) return EXIT_FAILURE;
        if (snprintf_check(tmp_fname, max_len, ".%c%i_%s", PATH_SEP, n++, file_name)) return EXIT_FAILURE;
    }
    strncpy(file_name, tmp_fname, max_len);
    file_name[max_len] = 0;
    return EXIT_SUCCESS;
}

int get_files_v1(sock_t socket) {
    int64_t cnt;
    if (read_size(socket, &cnt) != EXIT_SUCCESS) return EXIT_FAILURE;
    if (cnt <= 0) return EXIT_FAILURE;

    for (int64_t file_num = 0; file_num < cnt; file_num++) {
        int64_t name_length;
        if (read_size(socket, &name_length) != EXIT_SUCCESS) return EXIT_FAILURE;
        // limit file name length to 1024 chars
        if (name_length <= 0 || name_length > MAX_FILE_NAME_LENGTH) return EXIT_FAILURE;

        const uint64_t name_max_len = (uint64_t)(name_length + 16);
        if (name_max_len > MAX_FILE_NAME_LENGTH) {
            error("Too long file name.");
            return EXIT_FAILURE;
        }
        char file_name[name_max_len + 1];
        if (read_sock(socket, file_name, (uint64_t)name_length) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
        file_name[name_length] = 0;
        if (_is_valid_fname(file_name, (size_t)name_length) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
            printf("Invalid filename \'%s\'\n", file_name);
#endif
            return EXIT_FAILURE;
        }

        if (_get_base_name(file_name, (size_t)name_length) != EXIT_SUCCESS) return EXIT_FAILURE;

        // PATH_SEP is not allowed in file name
        if (strchr(file_name, PATH_SEP)) return EXIT_FAILURE;

        // if file already exists, use a different file name
        if (_rename_if_exists(file_name, name_max_len) != EXIT_SUCCESS) return EXIT_FAILURE;

        if (_save_file_common(1, socket, file_name) != EXIT_SUCCESS) return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int send_file_v1(sock_t socket) {
    list2 *file_list = get_copied_files();
    return _send_files_common(1, socket, file_list, 0);
    return EXIT_SUCCESS;
}

#endif

int get_image_v1(sock_t socket) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned long long t = (unsigned long long)(ts.tv_sec + ts.tv_nsec / 1000000);
    char file_name[40];
    snprintf(file_name, 35, "%llx.png", t);
    return _save_file_common(1, socket, file_name);
}

int info_v1(sock_t socket) {
    // TODO(thevindu-w): implement
    (void)socket;
    return EXIT_SUCCESS;
}

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
int get_files_v2(sock_t socket) {
    // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}

int send_files_v2(sock_t socket) {  // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
int get_copied_image_v3(sock_t socket) {  // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}

int get_screenshot_v3(sock_t socket) {
    // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}

int get_files_v3(sock_t socket) {
    // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}

int send_files_v3(sock_t socket) {  // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}
#endif
