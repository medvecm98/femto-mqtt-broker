#include "poll_list.h"

size_t
add_poll_fd(poll_list_t* list, int fd, conn_t *connection, short events) {
    int index_rv = list->index;
    if (list->size <= list->index + 1) {
        expand_list(list);
    }

    list->poll_fds[list->index].fd = fd;
    list->poll_fds[list->index].events = events;
    list->parent[list->index] = connection;
    list->index++;
    list->in_use++;
    return index_rv;
}

void
expand_list(poll_list_t *list) {
    list->poll_fds = realloc(
        list->poll_fds, sizeof(pollfd_t) * list->size * 2
    );

    if (!list->poll_fds) {
        err(1, "Expanding pollfd dynamic list failed. (pollfd_t)");
    }

    memset(list->poll_fds + list->size, 0, list->size * sizeof(pollfd_t));

    list->parent = realloc(
        list->parent, sizeof(conn_t*) * list->size * 2
    );

    if (!list->parent) {
        err(1, "Expanding pollfd dynamic list failed. (conn_t*)");
    }

    memset(list->parent + list->size, 0, list->size * sizeof(conn_t*));

    list->size *= 2;

    for (size_t i = list->index; i < list->size; i++) {
        list->poll_fds[i].fd = -1;
    }
}

void
shrink_list(poll_list_t *list) {
    list->poll_fds = realloc(
        list->poll_fds, sizeof(pollfd_t) * list->size / 2
    );
    if (!list->poll_fds) {
        err(1, "Shrinking pollfd dynamic list failed. (pollfd_t)");
    }

    list->parent = realloc(
        list->parent, sizeof(conn_t*) * list->size / 2
    );
    if (!list->parent) {
        err(1, "Shrinking pollfd dynamic list failed. (conn_t*)");
    }

    list->size /= 2;
}

void
shrink_list_checked(poll_list_t *list) {
    if (list->in_use * 4 < list->size) {
        shrink_list(list);
    }
}

void
poll_list_init(poll_list_t *list) {
    list->size = 16;
    list->index = 0;
    list->in_use = 0;
    list->poll_fds = calloc(16, sizeof(pollfd_t));
    list->parent = calloc(16, sizeof(conn_t*));

    for (size_t i = 0; i < list->size; i++) {
        list->poll_fds[i].fd = -1;
    }

    if (!list->poll_fds) {
        err(1, "calloc poll_list poll_list_init");
    }
}

void
poll_list_free(poll_list_t *list) {
    free(list->poll_fds);
    free(list->parent);
}

void
clear_entry(poll_list_t* list, size_t index) {
    if (!list->parent[index] && list->poll_fds[index].fd == -1) {
        return;
    }

    list->index--;

    if (index == list->index) {
        // last item can be just removed
        list->parent[list->index] = NULL;
        list->poll_fds[list->index].fd = -1;
    } else {
        // item that is not last needs to be swapped with last item
        memcpy(list->poll_fds + index, list->poll_fds + list->index, sizeof(pollfd_t));
        // set item being removed to last
        list->parent[index] = list->parent[list->index];
        list->parent[index]->poll_list_index = index;
        // remove last item
        list->parent[list->index] = NULL;
        list->poll_fds[list->index].fd = -1;
    }
    list->in_use--;
}
