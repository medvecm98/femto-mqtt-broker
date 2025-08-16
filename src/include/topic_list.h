#ifndef FEMTO_MQTT_TOPIC_LIST_H
#define FEMTO_MQTT_TOPIC_LIST_H
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "log.h"

/**
 * Topic linked list entry. Contains all topic-related data.
 */
struct topic {
    char **tokenized_topic; // array of tokens of tokenized topic
    char *topic; // topic in its string (pre-tokenization) form
    size_t topic_len; // length of the topic string (pre-tokenization)
    int topic_token_count; // how many tokens is topic composed of
    int qos_code; // qos code for this topic
    struct topic *next; // next topic in topic linked list
};

/**
 * Topic linked list entry. Contains all topic-related data.
 */
typedef struct topic topic_t;

/**
 * Topic linked list. Contains all topics that given client is subscribed to.
 */
struct topics {
    struct topic *head; // head of list (last inserted)
    struct topic *back; // back of list (first inserted)
};

typedef struct topics topics_t;

void
insert_topic(topics_t *list, char *topic_str, size_t topic_len, int qos_code);

int
remove_topic(topics_t *list, char *topic_str, size_t topic_len);

int
find_topic(topics_t *list, char *topic_str);

topics_t *
create_topics_list(void);

void
delete_topics_list(topics_t *list);

int
contains_wildcard_char(char *topic);

#endif
