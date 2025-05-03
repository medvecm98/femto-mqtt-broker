#include "poll_list.h"

size_t add_poll_fd(poll_list_t* list, int fd) {
    int index_rv = list->index++;
    if (list->size <= list->index) {
        expand_list(list);
    }

    list->poll_fds[list->index].fd = fd;
    return index_rv;
}

void expand_list(poll_list_t *list) {
    list->poll_fds = realloc(
        list->poll_fds, sizeof(struct pollfd) * list->size * 2
    );

    if (!list->poll_fds) {
        err(1, "Expanding pollfd dynamic list failed.");
    }

    memset(list->poll_fds + list->size, 0, list->size * sizeof(struct pollfd));
    list->size *= 2;
}

void shrink_list(poll_list_t *list) {
    list->poll_fds = realloc(
        list->poll_fds, sizeof(struct pollfd) * list->size / 2
    );

    if (!list->poll_fds) {
        err(1, "Shrinking pollfd dynamic list failed.");
    }

    list->size /= 2;
}

void shrink_list_checked(poll_list_t *list) {
    if (list->index * 4 < list->size) {
        shrink_list(list);
    }
}

void poll_list_init(poll_list_t *list) {
    list->size = 16;
    list->index = 0;
    list->poll_fds = calloc(16, sizeof(struct pollfd));

    if (!list->poll_fds) {
        err(1, "calloc poll_list poll_list_init");
    }
}