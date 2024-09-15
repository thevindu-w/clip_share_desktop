#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>
#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

sock_t connect_server(uint32_t server_addr, uint16_t port) {
    sock_t sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
#ifdef DEBUG_MODE
        fputs("Can\'t open socket\n", stderr);
#endif
        return -1;
    }
    struct sockaddr_in s_addr_in;
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = server_addr;
    s_addr_in.sin_port = htons(port);

    int status = connect(sock, (struct sockaddr *)&s_addr_in, sizeof(s_addr_in));
    if (status < 0) {
#ifdef DEBUG_MODE
        fputs("Connection failed\n", stderr);
#endif
        close_socket(sock);
        return -1;
    }

    return sock;
}

int read_sock(sock_t sock, char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_sz_read = 0;
    char *ptr = buf;
    while (total_sz_read < size) {
        ssize_t sz_read;
        sz_read = recv(sock, buf, size, 0);
        if (sz_read > 0) {
            total_sz_read += (uint64_t)sz_read;
            cnt = 0;
            ptr += sz_read;
        } else if (cnt++ > 50) {
#ifdef DEBUG_MODE
            fputs("Read sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int write_sock(sock_t sock, const char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_written = 0;
    const char *ptr = buf;
    while (total_written < size) {
        ssize_t sz_written;
        sz_written = send(sock, buf, size, 0);
        if (sz_written > 0) {
            total_written += (uint64_t)sz_written;
            cnt = 0;
            ptr += sz_written;
        } else if (cnt++ > 50) {
#ifdef DEBUG_MODE
            fputs("Write sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}


int send_size(sock_t socket, int64_t size) {
    char sz_buf[8];
    int64_t sz = size;
    for (int i = sizeof(sz_buf) - 1; i >= 0; i--) {
        sz_buf[i] = (char)(sz & 0xff);
        sz >>= 8;
    }
    return write_sock(socket, sz_buf, sizeof(sz_buf));
}

int read_size(sock_t socket, int64_t *size_ptr) {
    unsigned char sz_buf[8];
    if (read_sock(socket, (char *)sz_buf, sizeof(sz_buf)) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Read size failed\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    int64_t size = 0;
    for (unsigned i = 0; i < sizeof(sz_buf); i++) {
        size = (size << 8) | sz_buf[i];
    }
    *size_ptr = size;
    return EXIT_SUCCESS;
}


void close_socket(sock_t sock) {
#if defined(__linux__) || defined(__APPLE__)
    close(sock);
#elif defined(_WIN32)
    closesocket(sock);
#endif
}