#include <globals.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__) || defined(__APPLE__)
#define PATH_SEP '/'
#elif defined(_WIN32)
#define PATH_SEP '\\'
#endif

#define COMMAND_HELP 127
#define COMMAND_GET_TEXT 1
#define COMMAND_SEND_TEXT 2

// tcp and udp
#define APP_PORT 4337

// maximum transfer sizes
#define MAX_TEXT_LENGTH 4194304     // 4 MiB
#define MAX_FILE_SIZE 68719476736l  // 64 GiB

#define CONFIG_FILE "clipshare-desktop.conf"

config configuration;

static inline void print_usage(const char *prog_name) { fprintf(stderr, "Usage: %s COMMAND [ARGS]\n", prog_name); }

static inline void get_text() {
    // TODO: Implement
    puts("Get text done");
}

static inline void send_text() {
    // TODO: Implement
    puts("Send text done");
}

/*
 * Parse command line arguments and set corresponding variables
 */
static inline void _parse_args(int argc, char **argv, int8_t *command_p) {
    char cmd[4];
    strncpy(cmd, argv[1], 3);
    cmd[3] = 0;
    if (strncmp(cmd, "h", 2) == 0) {
        *command_p = COMMAND_HELP;
    } else if (strncmp(cmd, "g", 2) == 0) {
        *command_p = COMMAND_GET_TEXT;
    } else if (strncmp(cmd, "s", 2) == 0) {
        *command_p = COMMAND_SEND_TEXT;
    } else {
        fprintf(stderr, "Invalid command %s\n", argv[1]);
        *command_p = 0;
    }
}

/*
 * Apply default values to the configuration options that are not specified in conf file.
 */
static inline void _apply_default_conf(void) {
    if (configuration.app_port <= 0) configuration.app_port = APP_PORT;
    if (configuration.max_text_length <= 0) configuration.max_text_length = MAX_TEXT_LENGTH;
    if (configuration.max_file_size <= 0) configuration.max_file_size = MAX_FILE_SIZE;
}

/*
 * The main entrypoint of the application
 */
int main(int argc, char **argv) {
    // Get basename of the program
    const char *prog_name = strrchr(argv[0], PATH_SEP);
    if (!prog_name) {
        prog_name = argv[0];
    } else {
        prog_name++;  // don't want the '/' before the program name
    }

    if (argc <= 1) {
        print_usage(prog_name);
        return 1;
    }

    char *conf_path = strdup(CONFIG_FILE);
    parse_conf(&configuration, conf_path);
    free(conf_path);
    _apply_default_conf();
    
    int8_t command;
    _parse_args(argc, argv, &command);

    switch (command) {
        case COMMAND_HELP: {
            print_usage(prog_name);
            exit(0);
        }
        case COMMAND_GET_TEXT: {
            get_text();
            break;
        }
        case COMMAND_SEND_TEXT: {
            send_text();
            break;
        }
        default: {
            print_usage(prog_name);
            exit(1);
        }
    }

    return 0;
}