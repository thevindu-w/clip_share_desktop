#ifndef PROTO_METHODS_H_
#define PROTO_METHODS_H_

#include <utils/net_utils.h>

// status codes
#define STATUS_OK 1
#define STATUS_NO_DATA 2

// Version 1 methods
extern int get_text_v1(sock_t socket);
extern int send_text_v1(sock_t socket);
#if PROTOCOL_MIN <= 1
extern int get_files_v1(sock_t socket);
extern int send_file_v1(sock_t socket);
#endif
extern int get_image_v1(sock_t socket);
extern int info_v1(sock_t socket);

// Version 2 methods
#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
extern int get_files_v2(sock_t socket);
extern int send_files_v2(sock_t socket);
#endif

// Version 3 methods
#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
extern int get_files_v3(sock_t socket);
extern int send_files_v3(sock_t socket);
extern int get_copied_image_v3(sock_t socket);
extern int get_screenshot_v3(sock_t socket);
#endif

#endif  // PROTO_METHODS_H_
