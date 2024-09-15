#ifndef PROTO_SELECTOR_H_
#define PROTO_SELECTOR_H_

#include <utils/net_utils.h>

// methods
#define METHOD_GET_TEXT 1
#define METHOD_SEND_TEXT 2
#define METHOD_GET_FILE 3
#define METHOD_SEND_FILE 4
#define METHOD_GET_IMAGE 5
#define METHOD_GET_COPIED_IMAGE 6
#define METHOD_GET_SCREENSHOT 7
#define METHOD_INFO 125

/*
 * Runs the protocol client after the socket connection is established.
 * Accepts a socket, negotiates the protocol version, and passes the control
 * to the respective version handler.
 */
extern int handle_proto(sock_t socket, unsigned char method);

#endif  // PROTO_SELECTOR_H_
