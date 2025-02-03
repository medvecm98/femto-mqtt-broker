#ifndef FEMTO_MQTT_TOPIC_LIST_H
#define FEMTO_MQTT_TOPIC_LIST_H
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <err.h>
#include "log.h"

struct topic {
    char **tokenized_topic;
    char *topic;
    int topic_token_count;
    int qos_code;
    struct topic *next;
};

typedef struct topic topic_t;

struct topics {
    struct topic *head;
    struct topic *back;
};

typedef struct topics topics_t;

void
insert_topic(topics_t *list, char *topic_str, int qos_code);

int
insert_topic_checked(topics_t *list, char *topic_str, int qos_code);

int
remove_topic(topics_t *list, char *topic_str);

int
find_topic(topics_t *list, char *topic_str);

topics_t *
create_topics_list(void);

void
delete_topics_list(topics_t *list);

#endif
