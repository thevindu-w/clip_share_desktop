#ifndef UTILS_NET_UTILS_H_
#define UTILS_NET_UTILS_H_

#include <stdint.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
typedef int sock_t;
#elif defined(_WIN32)
typedef SOCKET sock_t;
#endif

extern sock_t connect_server(uint32_t server_addr, uint16_t port);

extern int read_sock(sock_t sock, char *buf, uint64_t size);

extern int write_sock(sock_t sock, const char *buf, uint64_t size);

extern int read_size(sock_t sock, int64_t *size_ptr);

extern int send_size(sock_t sock, int64_t num);

extern void close_socket(sock_t sock);

#endif  // UTILS_NET_UTILS_H_