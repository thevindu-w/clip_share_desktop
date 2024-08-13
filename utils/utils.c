#include <globals.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <utils/utils.h>
#ifdef __linux__
#include <X11/Xmu/Atoms.h>
#include <xclip/xclip.h>
#endif

int snprintf_check(char *dest, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(dest, size, fmt, ap);
    va_end(ap);
    return (ret < 0 || ret > (long)size);
}

void error(const char *msg) {
    if (!error_log_file) return;
#ifdef DEBUG_MODE
    fprintf(stderr, "%s\n", msg);
#endif
    FILE *f = open_file(error_log_file, "a");
    // retry with delays if failed
    for (unsigned int i = 0; i < 4; i++) {
        struct timespec interval = {.tv_sec = 0, .tv_nsec = (long)(1 + i * 50) * 1000000L};
        if (f != NULL || nanosleep(&interval, NULL)) break;
        f = open_file(error_log_file, "a");
    }
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
#if defined(__linux__) || defined(__APPLE__)
        chmod(error_log_file, S_IWUSR | S_IWGRP | S_IWOTH | S_IRUSR | S_IRGRP | S_IROTH);
#endif
    }
}

void error_exit(const char *msg) {
    error(msg);
    exit_wrapper(EXIT_FAILURE);
}

#ifdef __linux__
typedef struct _DisplayRec {
    struct _DisplayRec *next;
    Display *dpy;
    Atom atom;
} DisplayRec;

struct _AtomRec {
    _Xconst char *name;
    DisplayRec *head;
};

static void freeAtomPtr(AtomPtr atomPtr) {
    if (atomPtr) {
        DisplayRec *next = atomPtr->head;
        atomPtr->head = NULL;
        while (next) {
            DisplayRec *tmp = next->next;
            XtFree((char *)(next));
            next = tmp;
        }
    }
}
#endif

void exit_wrapper(int code) {
    if (error_log_file) free(error_log_file);
    if (cwd) free(cwd);
    clear_config(&configuration);
#ifdef __linux__
    freeAtomPtr(_XA_CLIPBOARD);
    freeAtomPtr(_XA_UTF8_STRING);
#endif
    exit(code);
}

#ifdef __linux__

int get_clipboard_text(char **buf_ptr, size_t *len_ptr) {
    if (xclip_util(XCLIP_OUT, NULL, len_ptr, buf_ptr) != EXIT_SUCCESS || *len_ptr <= 0) {  // do not change the order
#ifdef DEBUG_MODE
        printf("xclip read text failed. len = %zu\n", *len_ptr);
#endif
        if (*buf_ptr) free(*buf_ptr);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, size_t len) {
    if (xclip_util(XCLIP_IN, NULL, &len, &data) != EXIT_SUCCESS) {
#ifdef DEBUG_MODE
        fputs("Failed to write to clipboard\n", stderr);
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#endif
