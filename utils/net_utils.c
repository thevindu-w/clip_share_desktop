#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>
#include <utils/utils.h>
#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

int ipv4_aton(const char *address_str, uint32_t *address_ptr) {
    if (!address_ptr || !address_str) return EXIT_FAILURE;
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
    if (sscanf(address_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a >= 256 || b >= 256 || c >= 256 || d >= 256) {
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
    struct in_addr addr;
#if defined(__linux__) || defined(__APPLE__)
    if (inet_aton(address_str, &addr) != 1) {
#elif defined(_WIN32)
    if ((addr.s_addr = inet_addr(address_str)) == INADDR_NONE) {
#endif
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return EXIT_FAILURE;
    }
#if defined(__linux__) || defined(__APPLE__)
    *address_ptr = addr.s_addr;
#elif defined(_WIN32)
    *address_ptr = (uint32_t)addr.s_addr;
#endif
    return EXIT_SUCCESS;
}

sock_t connect_server(uint32_t addr, uint16_t port) {
    sock_t sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
#ifdef DEBUG_MODE
        fputs("Can\'t open socket\n", stderr);
#endif
        return INVALID_SOCKET;
    }
    // set timeout option to 5s
    struct timeval tv_connect = {5, 0};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv_connect, sizeof(tv_connect)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv_connect, sizeof(tv_connect))) {
        error("Can't set the timeout option of the connection");
        return INVALID_SOCKET;
    }

    struct sockaddr_in s_addr_in;
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = addr;
    s_addr_in.sin_port = htons(port);

    int status = connect(sock, (struct sockaddr *)&s_addr_in, sizeof(s_addr_in));
    if (status < 0) {
#ifdef DEBUG_MODE
        fputs("Connection failed\n", stderr);
#endif
        close_socket(sock);
        return INVALID_SOCKET;
    }

    // set timeout option to 100ms
    struct timeval tv = {0, 100000};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv))) {
        error("Can't set the timeout option of the connection");
        return INVALID_SOCKET;
    }

    return sock;
}

int read_sock(sock_t sock, char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_sz_read = 0;
    char *ptr = buf;
    while (total_sz_read < size) {
        ssize_t sz_read;
        uint64_t req_sz = size - total_sz_read;
        if (req_sz > 0x7fffffff) req_sz = 0x7fffffff;  // prevent overflow
#ifdef _WIN32
        sz_read = recv(sock, ptr, (int)req_sz, 0);
#else
        sz_read = recv(sock, ptr, req_sz, 0);
#endif
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
        uint64_t req_sz = size - total_written;
        if (req_sz > 0x7fffffff) req_sz = 0x7fffffff;  // prevent overflow
#ifdef _WIN32
        sz_written = send(sock, ptr, (int)req_sz, 0);
#else
        sz_written = send(sock, ptr, req_sz, 0);
#endif
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
