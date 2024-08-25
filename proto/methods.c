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
    // TODO (thevindu-w): implement
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

int get_image_v1(sock_t socket) {  // TODO (thevindu-w): implement
    return EXIT_SUCCESS;
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
