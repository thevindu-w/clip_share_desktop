#include <globals.h>
#include <proto/selector.h>
#include <proto/versions.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

// protocol version status
#define PROTOCOL_SUPPORTED 1
#define PROTOCOL_OBSOLETE 2
#define PROTOCOL_UNKNOWN 3

int handle_proto(sock_t socket, unsigned char method) {
    const unsigned short min_version = configuration.min_proto_version;
    const unsigned short max_version = configuration.max_proto_version;
    unsigned char version = (unsigned char)max_version;
    unsigned char status;

    if (write_sock(socket, (char *)&version, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "send protocol version failed\n");
#endif
        return EXIT_FAILURE;
    }

    if (read_sock(socket, (char *)&status, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fprintf(stderr, "read protocol version status failed\n");
#endif
        return EXIT_FAILURE;
    }

    switch (status) {
        case PROTOCOL_SUPPORTED: {  // protocol version accepted
            break;
        }

        case PROTOCOL_OBSOLETE: {
            error("Client's protocol versions are obsolete");
            return EXIT_FAILURE;
        }

        case PROTOCOL_UNKNOWN: {
            if (read_sock(socket, (char *)&version, 1) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
                fprintf(stderr, "negotiate protocol version failed\n");
#endif
                return EXIT_FAILURE;
            }
            if (version < min_version || version > max_version) {
                status = 0;
                write_sock(socket, (char *)&status, 1);  // Reject offer
                error("Protocol version negotiation failed");
                return EXIT_FAILURE;
            }
            break;
        }

        default: {
            error("Server sent invalid protocol version status");
            return EXIT_FAILURE;
        }
    }

    switch (version) {
#if PROTOCOL_MIN <= 1
        case 1: {
            return version_1(socket, method);
            break;
        }
#endif
#if (PROTOCOL_MIN <= 2) && (2 <= PROTOCOL_MAX)
        case 2: {
            return version_2(socket, method);
            break;
        }
#endif
#if (PROTOCOL_MIN <= 3) && (3 <= PROTOCOL_MAX)
        case 3: {
            return version_3(socket, method);
            break;
        }
#endif
        default: {  // invalid or unknown version
            error("Invalid protocol version");
            return EXIT_FAILURE;
        }
            return EXIT_FAILURE;
    }
}
