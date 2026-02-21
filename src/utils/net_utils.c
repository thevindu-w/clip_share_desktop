/*
 * utils/net_utils.c - platform specific implementation for socket connections
 * Copyright (C) 2022-2026 H. Thevindu J. Wijesekera
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

#include <errno.h>
#include <globals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/config.h>
#include <utils/net_utils.h>
#include <utils/utils.h>
#ifndef NO_SSL
#include <openssl/err.h>
#include <openssl/pkcs12.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#endif
#if defined(__linux__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#define close_sock(sock) close(sock)
#elif defined(_WIN32)
#define close_sock(sock) closesocket(sock);
#endif

#ifndef NO_SSL
static SSL_CTX *ctx = NULL;

void clear_ssl_ctx(void) {
    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
}

static int load_ssl_cert(const data_buffer *client_cert, const data_buffer *ca_cert) {
    BIO *sbio = BIO_new_mem_buf(client_cert->data, client_cert->len);
    PKCS12 *p12 = d2i_PKCS12_bio(sbio, NULL);
    if (!p12) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        BIO_free(sbio);
        return EXIT_FAILURE;
    }
    EVP_PKEY *key;
    X509 *client_x509;
    if (!PKCS12_parse(p12, "", &key, &client_x509, NULL)) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        BIO_free(sbio);
        PKCS12_free(p12);
        return EXIT_FAILURE;
    }
    BIO_free(sbio);
    PKCS12_free(p12);
    if (SSL_CTX_use_cert_and_key(ctx, client_x509, key, NULL, 1) != 1) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        EVP_PKEY_free(key);
        X509_free(client_x509);
        return EXIT_FAILURE;
    }
    EVP_PKEY_free(key);
    X509_free(client_x509);

    if (!SSL_CTX_check_private_key(ctx)) {
#ifdef DEBUG_MODE
        fputs("Private key does not match the public certificate\n", stderr);
#endif
        return EXIT_FAILURE;
    }

    BIO *cabio = BIO_new_mem_buf(ca_cert->data, ca_cert->len);
    X509 *ca_x509 = PEM_read_bio_X509(cabio, NULL, 0, NULL);
    BIO_free(cabio);
    X509_STORE *x509store = SSL_CTX_get_cert_store(ctx);
    if (X509_STORE_add_cert(x509store, ca_x509) != 1) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        X509_free(ca_x509);
        return EXIT_FAILURE;
    }
    X509_free(ca_x509);

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, 1);
    return EXIT_SUCCESS;
}

static inline void init_ssl_ctx(const data_buffer *client_cert, const data_buffer *ca_cert) {
    if (ctx) return;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *method = TLS_client_method();
    if (!method) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        return;
    }
    ctx = SSL_CTX_new(method);
    if (!ctx) {
#ifdef DEBUG_MODE
        ERR_print_errors_fp(stderr);
#endif
        return;
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    if (load_ssl_cert(client_cert, ca_cert) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Loading certificates failed\n", stderr);
#endif
        clear_ssl_ctx();
        return;
    }
}

static int check_peer_certs(const SSL *ssl, const list2 *trusted_servers) {
    X509 *cert;

    cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        return EXIT_FAILURE;
    }
    int verified = 0;
    char buf[256];
    X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, 255);
    buf[255] = 0;
#ifdef DEBUG_MODE
    printf("Server Common Name: %s\n", buf);
#endif
    for (size_t i = 0; i < trusted_servers->len; i++) {
        if (!strcmp(buf, trusted_servers->array[i])) {
            verified = 1;
#ifdef DEBUG_MODE
            puts("server verified");
#endif
            break;
        }
    }
    X509_free(cert);
    return verified ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

int validate_ipv4(const char *address_str) {
    if (!address_str) {
        return 1;
    }
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
    if (sscanf(address_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4 || a >= 256 || b >= 256 || c >= 256 || d >= 256) {
#ifdef DEBUG_MODE
        printf("Invalid address %s\n", address_str);
#endif
        return 1;
    }
    return 0;
}

int ipv4_aton(const char *address_str, uint32_t *address_ptr) {
    if ((!address_ptr) || validate_ipv4(address_str)) {
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

static void _connect_server(socket_t *sock_p, uint32_t addr) {
    sock_p->type = NULL_SOCK;
    sock_t sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
#ifdef DEBUG_MODE
        fputs("Can\'t open socket\n", stderr);
#endif
        return;
    }
    // set timeout option to 5s
#if defined(__linux__) || defined(__APPLE__)
    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
#elif defined(_WIN32)
    DWORD timeout = 5000;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))) {
        close_sock(sock);
        error("Can't set the timeout option of the connection");
        return;
    }

    struct sockaddr_in s_addr_in;
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = addr;
    const uint16_t port = configuration.secure_mode_enabled ? configuration.ports.tls : configuration.ports.plaintext;
    s_addr_in.sin_port = htons(port);

    int status = connect(sock, (struct sockaddr *)&s_addr_in, sizeof(s_addr_in));
    if (status < 0) {
#ifdef DEBUG_MODE
        fputs("Connection failed\n", stderr);
#endif
        close_sock(sock);
        return;
    }

    // set timeout option to 0.5s
#if defined(__linux__) || defined(__APPLE__)
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000L;
#elif defined(_WIN32)
    timeout = 500;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))) {
        close_sock(sock);
        error("Can't set the timeout option of the connection");
        return;
    }

    int type = VALID_SOCK;
    type |= configuration.secure_mode_enabled ? SSL_SOCK : PLAIN_SOCK;
    const unsigned char sock_type = (unsigned char)type;
    if (!IS_SSL(sock_type)) {
        sock_p->socket.plain = sock;
        sock_p->type = sock_type;
        return;
    }

#ifndef NO_SSL
    init_ssl_ctx(&(configuration.client_cert), &(configuration.ca_cert));
    if (!ctx) {
        error("SSL CTX init failed");
        return;
    }
    SSL *ssl = SSL_new(ctx);
    SSL_set_min_proto_version(ssl, TLS1_2_VERSION);
    int ret;
#ifdef _WIN32
    ret = SSL_set_fd(ssl, (int)sock);
#else
    ret = SSL_set_fd(ssl, sock);
#endif
    if (ret != 1) {
        SSL_free(ssl);
        close_sock(sock);
        return;
    }

    if (SSL_connect(ssl) != 1) {
#ifdef DEBUG_MODE
        fputs("SSL_connect error\n", stderr);
        ERR_print_errors_fp(stderr);
#endif
        SSL_free(ssl);
        close_sock(sock);
        return;
    }
    sock_p->socket.ssl = ssl;
    sock_p->type = sock_type;
    if (check_peer_certs(ssl, configuration.trusted_servers) != EXIT_SUCCESS) {
        close_socket_no_wait(sock_p);
        return;
    }
#else
    close_sock(sock);
    error("Requesting SSL connection in NO_SSL version");
#endif
}

void connect_server(socket_t *sock_p, uint32_t addr) {
    _connect_server(sock_p, addr);
    if (IS_NULL_SOCK(sock_p->type)) {
        _connect_server(sock_p, addr);
    }
}

void get_udp_socket(socket_t *sock_p) {
    sock_p->type = NULL_SOCK;
    sock_t sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        return;
    }
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast))) {
        close_sock(sock);
        return;
    }
    // set timeout option to 2s
#if defined(__linux__) || defined(__APPLE__)
    struct timeval timeout = {.tv_sec = 2, .tv_usec = 0};
#elif defined(_WIN32)
    DWORD timeout = 2000;
#endif
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout))) {
        close_sock(sock);
        return;
    }
    sock_p->socket.plain = sock;
    sock_p->type = VALID_SOCK | TRNSPRT_UDP;
}

static inline ssize_t _read_plain(sock_t sock, char *buf, size_t size, int *fatal_p) {
    ssize_t sz_read;
#ifdef _WIN32
    sz_read = recv(sock, buf, (int)size, 0);
    if (sz_read == 0) {
        *fatal_p = 1;
    } else if (sz_read < 0) {
        int err_code = WSAGetLastError();
        if (err_code == WSAEBADF || err_code == WSAECONNREFUSED || err_code == WSAECONNRESET ||
            err_code == WSAECONNABORTED || err_code == WSAESHUTDOWN || err_code == WSAEDISCON ||
            err_code == WSAEHOSTDOWN || err_code == WSAEHOSTUNREACH || err_code == WSAENETRESET ||
            err_code == WSAENETDOWN || err_code == WSAENETUNREACH || err_code == WSAEFAULT || err_code == WSAEINVAL ||
            err_code == WSA_NOT_ENOUGH_MEMORY || err_code == WSAENOTCONN || err_code == WSANOTINITIALISED ||
            err_code == WSASYSCALLFAILURE || err_code == WSAEOPNOTSUPP || err_code == WSAENOTSOCK) {
            *fatal_p = 1;
        }
    }
#else
    errno = 0;
    sz_read = recv(sock, buf, size, 0);
    if (sz_read == 0 ||
        (sz_read < 0 && (errno == EBADF || errno == ECONNREFUSED || errno == ECONNRESET || errno == ECONNABORTED ||
                         errno == ESHUTDOWN || errno == EHOSTDOWN || errno == EHOSTUNREACH || errno == ENETRESET ||
                         errno == ENETDOWN || errno == ENETUNREACH || errno == EFAULT || errno == EINVAL ||
                         errno == ENOMEM || errno == ENOTCONN || errno == EOPNOTSUPP || errno == ENOTSOCK))) {
        *fatal_p = 1;
    }
#endif
    return sz_read;
}

#ifndef NO_SSL
static inline int _read_SSL(SSL *ssl, char *buf, int size, int *fatal_p) {
    int sz_read = SSL_read(ssl, buf, size);
    if (sz_read <= 0) {
        int err_code = SSL_get_error(ssl, sz_read);
        if (err_code == SSL_ERROR_ZERO_RETURN || err_code == SSL_ERROR_SYSCALL || err_code == SSL_ERROR_SSL) {
            *fatal_p = 1;
        }
    }
    return sz_read;
}
#endif

int read_sock(socket_t *socket, char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_sz_read = 0;
    char *ptr = buf;
    while (total_sz_read < size) {
        ssize_t sz_read;
        int fatal = 0;
        uint64_t read_req_sz = size - total_sz_read;
        if (read_req_sz > 0x7FFFFFFFL) read_req_sz = 0x7FFFFFFFL;  // prevent overflow due to casting
        if (!IS_SSL(socket->type)) {
            sz_read = _read_plain(socket->socket.plain, ptr, (uint32_t)read_req_sz, &fatal);
#ifndef NO_SSL
        } else {
            sz_read = _read_SSL(socket->socket.ssl, ptr, (int)read_req_sz, &fatal);
#else
        } else {
            return EXIT_FAILURE;
#endif
        }
        if (sz_read > 0) {
            total_sz_read += (uint64_t)sz_read;
            cnt = 0;
            ptr += sz_read;
        } else if (cnt > 10 || fatal) {
#ifdef DEBUG_MODE
            fputs("Read sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
        cnt++;
    }
    return EXIT_SUCCESS;
}

static inline ssize_t _write_plain(sock_t sock, const char *buf, size_t size, int *fatal_p) {
    ssize_t sz_written;
#ifdef _WIN32
    sz_written = send(sock, buf, (int)size, 0);
    if (sz_written < 0) {
        int err_code = WSAGetLastError();
        if (err_code == WSAEBADF || err_code == WSAECONNREFUSED || err_code == WSAECONNRESET ||
            err_code == WSAECONNABORTED || err_code == WSAESHUTDOWN || err_code == WSAEISCONN ||
            err_code == WSAEDESTADDRREQ || err_code == WSAEDISCON || err_code == WSAEHOSTDOWN ||
            err_code == WSAEHOSTUNREACH || err_code == WSAENETRESET || err_code == WSAENETDOWN ||
            err_code == WSAENETUNREACH || err_code == WSAEFAULT || err_code == WSAEINVAL ||
            err_code == WSA_NOT_ENOUGH_MEMORY || err_code == WSAENOTCONN || err_code == WSANOTINITIALISED ||
            err_code == WSASYSCALLFAILURE || err_code == WSAEOPNOTSUPP || err_code == WSAENOTSOCK) {
            *fatal_p = 1;
        }
    }
#else
    errno = 0;
    sz_written = send(sock, buf, size, 0);
    if (sz_written < 0 &&
        (errno == EBADF || errno == ECONNREFUSED || errno == ECONNRESET || errno == ECONNABORTED ||
         errno == ESHUTDOWN || errno == EPIPE || errno == EISCONN || errno == EDESTADDRREQ || errno == EHOSTDOWN ||
         errno == EHOSTUNREACH || errno == ENETRESET || errno == ENETDOWN || errno == ENETUNREACH || errno == EFAULT ||
         errno == EINVAL || errno == ENOMEM || errno == ENOTCONN || errno == EOPNOTSUPP || errno == ENOTSOCK)) {
        *fatal_p = 1;
    }
#endif
    return sz_written;
}

#ifndef NO_SSL
static inline int _write_SSL(SSL *ssl, const char *buf, int size, int *fatal_p) {
    int sz_written = SSL_write(ssl, buf, size);
    if (sz_written <= 0) {
        int err_code = SSL_get_error(ssl, sz_written);
        if (err_code == SSL_ERROR_ZERO_RETURN || err_code == SSL_ERROR_SYSCALL || err_code == SSL_ERROR_SSL) {
            *fatal_p = 1;
        }
    }
    return sz_written;
}
#endif

int write_sock(socket_t *socket, const char *buf, uint64_t size) {
    int cnt = 0;
    uint64_t total_written = 0;
    const char *ptr = buf;
    while (total_written < size) {
        ssize_t sz_written;
        int fatal = 0;
        uint64_t write_req_sz = size - total_written;
        if (write_req_sz > 0x7FFFFFFFL) write_req_sz = 0x7FFFFFFFL;  // prevent overflow due to casting
        if (!IS_SSL(socket->type)) {
            sz_written = _write_plain(socket->socket.plain, ptr, (uint32_t)write_req_sz, &fatal);
#ifndef NO_SSL
        } else {
            sz_written = _write_SSL(socket->socket.ssl, ptr, (int)write_req_sz, &fatal);
#else
        } else {
            return EXIT_FAILURE;
#endif
        }
        if (sz_written > 0) {
            total_written += (uint64_t)sz_written;
            cnt = 0;
            ptr += sz_written;
        } else if (cnt > 10 || fatal) {
#ifdef DEBUG_MODE
            fputs("Write sock failed\n", stderr);
#endif
            return EXIT_FAILURE;
        }
        cnt++;
    }
    return EXIT_SUCCESS;
}

int send_size(socket_t *socket, int64_t size) {
    char sz_buf[8];
    int64_t sz = size;
    for (int i = sizeof(sz_buf) - 1; i >= 0; i--) {
        sz_buf[i] = (char)(sz & 0xff);
        sz >>= 8;
    }
    return write_sock(socket, sz_buf, sizeof(sz_buf));
}

int read_size(socket_t *socket, int64_t *size_ptr) {
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

void _close_socket(socket_t *socket, int await) {
    if (IS_NULL_SOCK(socket->type)) return;
    if (!IS_SSL(socket->type)) {
        if (await) {
            char tmp;
            recv(socket->socket.plain, &tmp, 1, 0);
        }
        close_sock(socket->socket.plain);
#ifndef NO_SSL
    } else {
        sock_t sd;
#ifdef _WIN32
        sd = (sock_t)SSL_get_fd(socket->socket.ssl);
#else
        sd = SSL_get_fd(socket->socket.ssl);
#endif
        int err;
        if (await) {
            char tmp;
            int ret = SSL_read(socket->socket.ssl, &tmp, 1);
            err = SSL_get_error(socket->socket.ssl, ret);
        } else {
            err = SSL_ERROR_NONE;
        }
        if (err != SSL_ERROR_SYSCALL && err != SSL_ERROR_SSL) {
            SSL_shutdown(socket->socket.ssl);
        }
        SSL_free(socket->socket.ssl);
        close_sock(sd);
#endif
    }
    socket->type = NULL_SOCK;
}
