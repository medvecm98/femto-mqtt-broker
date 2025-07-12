#ifndef FEMTO_MQTT_POLL_H
#define FEMTO_MQTT_POLL_H

#include <poll.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <structs.h>

typedef struct pollfd pollfd_t;

typedef struct {
    pollfd_t *poll_fds;
    size_t index; // points to first free/next-to-be-used entry in the list
    size_t size; // size of the space allocated for the list
    size_t in_use; // how many entries are actually used (so not -1)
    conn_t **parent; // connectios polls are associated with
} poll_list_t;

typedef poll_list_t* plist_ptr;

void poll_list_init(poll_list_t* list);

void poll_list_free(poll_list_t* list);

size_t add_poll_fd(poll_list_t* list, int fd, conn_t *connection, short events);

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

void clear_entry(poll_list_t* list, size_t index);

#endif