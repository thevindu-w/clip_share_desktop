#ifndef PROTO_VERSIONS_H_
#define PROTO_VERSIONS_H_

#include <utils/net_utils.h>

#if PROTOCOL_MIN <= 1
/*
 * Accepts a socket connection and method code after the protocol version 1 is selected after the negotiation phase.
 * Negotiate the method code with the server and pass the control to the respective method handler.
 */
extern int version_1(sock_t socket, unsigned char method);
#endif

#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
/*
 * Accepts a socket connection and method code after the protocol version 2 is selected after the negotiation phase.
 * Negotiate the method code with the server and pass the control to the respective method handler.
 */
extern int version_2(sock_t socket, unsigned char method);
#endif

#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
/*
 * Accepts a socket connection and method code after the protocol version 3 is selected after the negotiation phase.
 * Negotiate the method code with the server and pass the control to the respective method handler.
 */
extern int version_3(sock_t socket, unsigned char method);
#endif

#endif  // PROTO_VERSIONS_H_
