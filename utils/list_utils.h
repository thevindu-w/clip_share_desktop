#ifndef UTILS_LIST_UTILS_H_
#define UTILS_LIST_UTILS_H_

#include <stdlib.h>

typedef struct _list {
    size_t len;
    size_t capacity;
    void **array;
} list2;

/*
 * Initialize a list2 with initial capacity len
 * returns NULL on error
 */
extern list2 *init_list(size_t len);

/*
 * Free the memory allocated to a list2 *lst
 */
extern void free_list(list2 *lst);

/*
 * Appends the element elem to the list lst.
 * Allocates more space if needed.
 */
extern void append(list2 *lst, void *elem);

#endif  // UTILS_LIST_UTILS_H_
