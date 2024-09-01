#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <globals.h>
#include <stdio.h>
#include <unistd.h>

#if defined(__linux__) || defined(__APPLE__)
#define PATH_SEP '/'
#elif defined(_WIN32)
#define PATH_SEP '\\'
#endif

/*
 * A wrapper for snprintf.
 * returns 1 if snprintf failed or truncated
 * returns 0 otherwise
 */
int snprintf_check(char *dest, size_t size, const char *fmt, ...)
#ifdef _WIN32
    __attribute__((__format__(gnu_printf, 3, 4)));
#else
    __attribute__((__format__(printf, 3, 4)));
#endif

/*
 * Append error message to error log file
 */
extern void error(const char *msg);

/*
 * Append error message to error log file and exit
 */
extern void error_exit(const char *msg) __attribute__((noreturn));

/*
 * Exit after clearing config and other memory allocations
 */
void exit_wrapper(int code) __attribute__((noreturn));

/*
 * Get copied text from clipboard.
 * Places the text in a buffer and sets the bufptr to point the buffer.
 * bufptr must be a valid pointer to a char * variable.
 * Caller should free the buffer after using.
 * Places text length in the memory location pointed to by lenptr.
 * lenptr must be a valid pointer to a size_t variable.
 * On failure, buffer is set to NULL.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int get_clipboard_text(char **bufptr, size_t *lenptr);

/*
 * Reads len bytes of text from the data buffer and copies it into the clipboard.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int put_clipboard_text(char *data, size_t len);

/*
 * Check if a file exists at the path given by file_name.
 * returns 1 if a file or directory or other special file type exists or 0 otherwise.
 */
extern int file_exists(const char *file_name);

/*
 * Check if the file at path is a directory.
 * returns 1 if its a directory or 0 otherwise
 */
extern int is_directory(const char *path, int follow_symlinks);

/*
 * Converts line endings to LF or CRLF based on the platform.
 * param str_p is a valid pointer to malloced, null-terminated char * which may be realloced and returned.
 * If force_lf is non-zero, convert EOL to LF regardless of the platform
 * Else, convert EOL of str to LF.
 * Returns the length of the new string without the terminating null character.
 * If an error occured, this will free() the *str_p and return -1.
 */
extern int64_t convert_eol(char **str_p, int force_lf);

#if defined(__linux__) || defined(__APPLE__)

#define open_file(filename, mode) fopen(filename, mode)
#define remove_file(filename) remove(filename)
#define getcwd_wrapper(len) getcwd(NULL, len)

#elif defined(_WIN32)

/**
 * Get the pathname of the current working directory. If successful, returns an array which is allocated with malloc;
 * the array is at least len bytes long.
 * Returns NULL if the directory couldn't be determined.
 */
extern char *getcwd_wrapper(int len);

/*
 * A wrapper for fopen() to be platform independent.
 * Internally converts the filename to wide char on Windows.
 */
extern FILE *open_file(const char *filename, const char *mode);

/*
 * A wrapper for remove() to be platform independent.
 * Internally converts the filename to wide char on Windows.
 */
extern int remove_file(const char *filename);

#endif

#if (PROTOCOL_MIN <= 3) && (2 <= PROTOCOL_MAX)

/*
 * Creates the directory given by the path and all its parent directories if missing.
 * Will not delete any existing files or directories.
 * returns EXIT_SUCCESS on success and EXIT_FAILURE on failure.
 */
extern int mkdirs(const char *path);

#endif

#endif  // UTILS_UTILS_H_
