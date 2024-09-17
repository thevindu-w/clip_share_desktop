#include <client.h>
#include <globals.h>
#include <proto/selector.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>

static int _invoke_method(unsigned char method) {
    sock_t sock = connect_server(server_addr, configuration.app_port);
    if (sock <= 0) return EXIT_FAILURE;
    int ret = handle_proto(sock, method);
    close_socket(sock);
    return ret;
}

void get_text() {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_TEXT) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get text %s\n", msg_suffix);
}

void send_text() {
    const char *msg_suffix;
    if (_invoke_method(METHOD_SEND_TEXT) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Send text %s\n", msg_suffix);
}

void get_files() {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_FILE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get files %s\n", msg_suffix);
}

void send_files() {
    const char *msg_suffix;
    if (_invoke_method(METHOD_SEND_FILE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Send files %s\n", msg_suffix);
}

void get_image() {
    const char *msg_suffix;
    if (_invoke_method(METHOD_GET_IMAGE) == EXIT_SUCCESS)
        msg_suffix = "done";
    else
        msg_suffix = "failed!";
    printf("Get image %s\n", msg_suffix);
}
