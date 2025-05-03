#ifndef FEMTO_MQTT_POLL_H
#define FEMTO_MQTT_POLL_H

#include <poll.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

typedef struct {
    struct pollfd *poll_fds;
    size_t index; // points to last used entry in the list
    size_t size; // size of the space allocated for the list
} poll_list_t;

void poll_list_init(poll_list_t* list);

void poll_list_free(poll_list_t* list);

size_t add_poll_fd(poll_list_t* list, int fd);

/**
 * Expands the list by doubling the allocated memory. Do this when list is
 * full.
 */
void expand_list(poll_list_t*);

/**
 * Shrinks the list by halving the allocated memory. Ideally, do this when
 * three quarters of the list are empty.
 */
void shrink_list(poll_list_t*);

/**
 * Shrinks the list by halving the allocated memory. Ideally, do this when
 * three quarters of the list are empty.
 */
void shrink_list_checked(poll_list_t*);

#endif