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

/*
 * Common function to save files in get_files and get_image methods.
 */
static int _save_file_common(int version, sock_t socket, const char *file_name);

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

static int _save_file_common(int version, sock_t socket, const char *file_name) {
    int64_t file_size;
    if (read_size(socket, &file_size) != EXIT_SUCCESS) return EXIT_FAILURE;
#ifdef DEBUG_MODE
    printf("data len = %lli\n", (long long)file_size);
#endif
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

#if PROTOCOL_MIN <= 1

int get_files_v1(sock_t socket) {
    // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
}

int send_file_v1(sock_t socket) {
    // TODO (thevindu-w): implement
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
    // TODO (thevindu-w): implement
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
