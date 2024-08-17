#ifndef PROTO_SELECTOR_H_
#define PROTO_SELECTOR_H_

#include <utils/net_utils.h>

/*
 * Runs the protocol client after the socket connection is established.
 * Accepts a socket, negotiates the protocol version, and passes the control
 * to the respective version handler.
 */
extern int handle_proto(sock_t socket, unsigned char method);

#endif  // PROTO_SELECTOR_H_
