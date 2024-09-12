#include <stdio.h>
#include <stdlib.h>
#include <utils/list_utils.h>
#include <utils/utils.h>

list2 *init_list(size_t len) {
    list2 *lst = (list2 *)malloc(sizeof(list2));
    if (!lst) return NULL;
    void **arr = (void **)malloc(len * sizeof(void *));
    if (!arr) {
        free(lst);
        return NULL;
    }
    lst->array = arr;
    lst->len = 0;
    lst->capacity = len;
    return lst;
}

void free_list(list2 *lst) {
    for (size_t i = 0; i < lst->len; i++) {
        if (lst->array[i]) free(lst->array[i]);
    }
    free(lst->array);
    free(lst);
}

void append(list2 *lst, void *elem) {
    if (lst->len >= lst->capacity) {
        size_t new_cap;
        if (lst->capacity < 0x80000000L) {
            new_cap = lst->capacity * 2;
        } else if (lst->capacity >= 0xFFFFFFFFL) {
            return;
        } else {
            new_cap = 0xFFFFFFFFL;
        }
        void **new_arr = (void **)realloc(lst->array, sizeof(void *) * new_cap);
        if (!new_arr) return;
        lst->array = new_arr;
        lst->capacity = new_cap;
    }
    lst->array[lst->len] = elem;
    lst->len++;
}
