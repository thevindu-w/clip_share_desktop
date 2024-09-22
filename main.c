#include <client.h>
#include <globals.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#define COMMAND_HELP 127
#define COMMAND_GET_TEXT 1
#define COMMAND_SEND_TEXT 2
#define COMMAND_GET_FILES 3
#define COMMAND_SEND_FILES 4
#define COMMAND_GET_IMAGE 5

// tcp and udp
#define APP_PORT 4337

// maximum transfer sizes
#define MAX_TEXT_LENGTH 4194304     // 4 MiB
#define MAX_FILE_SIZE 68719476736l  // 64 GiB

#define ERROR_LOG_FILE "client_err.log"
#define CONFIG_FILE "clipshare-desktop.conf"

config configuration;
char *error_log_file;
char *cwd;
size_t cwd_len;
uint32_t server_addr;

static inline void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s <server-address-ipv4> COMMAND\n", prog_name);
    fprintf(stderr,
            "Commands available:\n"
            "\th  : Help\n"
            "\tg  : Get copied text\n"
            "\ts  : Send copied text\n"
            "\tfg : Get copied files\n"
            "\tfs : Send copied files\n"
            "\ti  : Get image\n");
    fprintf(stderr,
            "\nExample: %s  192.168.21.42  g\n"
            "\tThis command gets copied text from the device having IP address 192.168.21.42\n\n",
            prog_name);
}

/*
 * Parse command line arguments and set corresponding variables
 */
static inline void _parse_args(char **argv, int8_t *command_p) {
    if (ipv4_aton(argv[1], &server_addr) != EXIT_SUCCESS) {
        fprintf(stderr, "Invalid server address %s\n", argv[1]);
        *command_p = 0;
        return;
    }
    char cmd[4];
    strncpy(cmd, argv[2], 3);
    cmd[3] = 0;
    if (strncmp(cmd, "h", 2) == 0) {
        *command_p = COMMAND_HELP;
    } else if (strncmp(cmd, "g", 2) == 0) {
        *command_p = COMMAND_GET_TEXT;
    } else if (strncmp(cmd, "s", 2) == 0) {
        *command_p = COMMAND_SEND_TEXT;
    } else if (strncmp(cmd, "fg", 3) == 0) {
        *command_p = COMMAND_GET_FILES;
    } else if (strncmp(cmd, "fs", 3) == 0) {
        *command_p = COMMAND_SEND_FILES;
    } else if (strncmp(cmd, "i", 2) == 0) {
        *command_p = COMMAND_GET_IMAGE;
    } else {
        fprintf(stderr, "Invalid command %s\n", argv[2]);
        *command_p = 0;
    }
}

/*
 * Set the error_log_file absolute path
 */
static inline void _set_error_log_file(const char *path) {
    char *working_dir = getcwd_wrapper(2050);
    if (!working_dir) exit_wrapper(EXIT_FAILURE);
    working_dir[2049] = 0;
    size_t working_dir_len = strnlen(working_dir, 2048);
    if (working_dir_len == 0 || working_dir_len >= 2048) {
        free(working_dir);
        exit_wrapper(EXIT_FAILURE);
    }
    size_t buf_sz = working_dir_len + strlen(path) + 1;  // +1 for terminating \0
    if (working_dir[working_dir_len - 1] != PATH_SEP) {
        buf_sz++;  // 1 more char for PATH_SEP
    }
    error_log_file = malloc(buf_sz);
    if (!error_log_file) {
        free(working_dir);
        exit_wrapper(EXIT_FAILURE);
    }
    if (working_dir[working_dir_len - 1] == PATH_SEP) {
        snprintf_check(error_log_file, buf_sz, "%s%s", working_dir, ERROR_LOG_FILE);
    } else {
        snprintf_check(error_log_file, buf_sz, "%s/%s", working_dir, ERROR_LOG_FILE);
    }
    free(working_dir);
}

/*
 * Change working directory to the directory specified in the configuration
 */
static inline void _change_working_dir(void) {
    if (!is_directory(configuration.working_dir, 1)) {
        char err[3072];
        snprintf_check(err, 3072, "Not existing working directory \'%s\'", configuration.working_dir);
        fprintf(stderr, "%s\n", err);
        error_exit(err);
    }
    char *old_work_dir = getcwd_wrapper(0);
    if (chdir_wrapper(configuration.working_dir)) {
        char err[3072];
        snprintf_check(err, 3072, "Failed changing working directory to \'%s\'", configuration.working_dir);
        fprintf(stderr, "%s\n", err);
        if (old_work_dir) free(old_work_dir);
        error_exit(err);
    }
    char *new_work_dir = getcwd_wrapper(0);
    if (old_work_dir == NULL || new_work_dir == NULL) {
        const char *err = "Error occured during changing working directory.";
        fprintf(stderr, "%s\n", err);
        if (old_work_dir) free(old_work_dir);
        if (new_work_dir) free(new_work_dir);
        error_exit(err);
    }
    // if the working directory did not change, set configuration.working_dir to NULL
    if (!strcmp(old_work_dir, new_work_dir)) {
        configuration.working_dir = NULL;
    }
    free(old_work_dir);
    free(new_work_dir);
}

/*
 * Apply default values to the configuration options that are not specified in conf file.
 */
static inline void _apply_default_conf(void) {
    if (configuration.app_port <= 0) configuration.app_port = APP_PORT;
    if (configuration.max_text_length <= 0) configuration.max_text_length = MAX_TEXT_LENGTH;
    if (configuration.max_file_size <= 0) configuration.max_file_size = MAX_FILE_SIZE;
    if (configuration.min_proto_version < PROTOCOL_MIN) configuration.min_proto_version = PROTOCOL_MIN;
    if (configuration.min_proto_version > PROTOCOL_MAX) configuration.min_proto_version = PROTOCOL_MAX;
    if (configuration.max_proto_version < configuration.min_proto_version ||
        configuration.max_proto_version > PROTOCOL_MAX)
        configuration.max_proto_version = PROTOCOL_MAX;
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

    if (argc < 3) {
        print_usage(prog_name);
        return 1;
    }

    _set_error_log_file(ERROR_LOG_FILE);

    char *conf_path = strdup(CONFIG_FILE);
    parse_conf(&configuration, conf_path);
    free(conf_path);
    _apply_default_conf();

    int8_t command;
    _parse_args(argv, &command);

    if (configuration.working_dir) _change_working_dir();
    cwd = getcwd_wrapper(0);
    cwd_len = strnlen(cwd, 2048);

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
        case COMMAND_GET_FILES: {
            get_files();
            break;
        }
        case COMMAND_SEND_FILES: {
            send_files();
            break;
        }
        case COMMAND_GET_IMAGE: {
            get_image();
            break;
        }
        default: {
            print_usage(prog_name);
            exit(1);
        }
    }

    return 0;
}
