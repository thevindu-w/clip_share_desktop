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

/*
 * Allocate the required capacity for the string with EOL=CRLF including the terminating '\0'.
 * Assign the realloced string to *str_p and the length after conversion to *len_p without the terminating '\0'.
 * Returns 0 if all \n are preceded by \r (i.e. no conversion needed).
 * Returns 1 if conversion is needed and it will increase the length.
 * Returns -1 if realloc failed.
 */
static inline int _realloc_for_crlf(char **str_p, uint64_t *len_p) {
    char *str = *str_p;
    size_t increase = 0;
    size_t ind;
    for (ind = 0; str[ind]; ind++) {
        if (str[ind] == '\n' && (ind == 0 || str[ind - 1] != '\r')) {
            // needs increasing
            increase++;
        }
    }
    if (!increase) {
        *len_p = ind;
        return 0;
    }
    uint64_t req_len = ind + increase;
    char *re_str = realloc(str, req_len + 1);  // +1 for terminating '\0'
    if (!re_str) {
        free(str);
        error("realloc failed");
        return -1;
    }
    *str_p = re_str;
    *len_p = req_len;
    return 1;
}

static inline void _convert_to_crlf(char *str, uint64_t new_len) {
    // converting to CRLF expands string. Therefore, start from the end to avoid overwriting
    size_t new_ind = new_len - 1;
    str[new_len] = 0;  // terminating '\0'
    for (size_t cur_ind = strnlen(str, new_len + 1) - 1;; cur_ind--) {
        char c = str[cur_ind];
        str[new_ind--] = c;
        if (c == '\n' && (cur_ind == 0 || str[cur_ind - 1] != '\r')) {
            // add the missing \r before \n
            str[new_ind--] = '\r';
        }
        if (cur_ind == 0) break;
    }
}

static inline int64_t _convert_to_lf(char *str) {
    // converting to CRLF shrinks string. Therefore, start from the begining to avoid overwriting
    const char *old_ptr = str;
    char *new_ptr = str;
    char c;
    while ((c = *old_ptr)) {
        old_ptr++;
        if (c != '\r' || *old_ptr != '\n') {
            *new_ptr = c;
            new_ptr++;
        }
    }
    *new_ptr = '\0';
    return new_ptr - str;
}

int64_t convert_eol(char **str_p, int force_lf) {
    int crlf;
#if defined(__linux__) || defined(__APPLE__)
    crlf = 0;
#elif defined(_WIN32)
    crlf = 1;
#endif
    if (force_lf) crlf = 0;
    // realloc if available capacity is not enough
    if (crlf) {
        uint64_t new_len;
        int status = _realloc_for_crlf(str_p, &new_len);
        if (status == 0) return (int64_t)new_len;  // no conversion needed
        if (status < 0 || !*str_p) return -1;      // realloc failed
        _convert_to_crlf(*str_p, new_len);
        return (int64_t)new_len;
    }
    return _convert_to_lf(*str_p);
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
