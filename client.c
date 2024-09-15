#include <globals.h>
#include <proto/selector.h>
#include <stdio.h>
#include <utils/net_utils.h>

// TODO(thevindu-w): change the hardcoded server address
uint32_t server_addr = 0x100007f;

static void _invoke_method(unsigned char method) {
    sock_t sock = connect_server(server_addr, configuration.app_port);
    if (sock <= 0) return;
    handle_proto(sock, method);
    close_socket(sock);
}

void get_text() {
    _invoke_method(METHOD_GET_TEXT);
    puts("Get text done");
}

void send_text() {
    _invoke_method(METHOD_SEND_TEXT);
    puts("Send text done");
}

void get_files() {
    _invoke_method(METHOD_GET_FILE);
    puts("Get files done");
}

void send_files() {
    _invoke_method(METHOD_SEND_FILE);
    puts("Send files done");
}

void get_image() {
    _invoke_method(METHOD_GET_IMAGE);
    puts("Get image done");
}
