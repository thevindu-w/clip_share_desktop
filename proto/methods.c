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
    // TODO (thevindu-w): implement
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
