#include "topic_list.h"

char**
tokenize_topic(char *topic, int *count_out) {
    char *temp = topic;
    int count = 1;
    while ((temp = strstr(temp, "/"))) {
        count++;
        temp++;
    }

    char **tokenized_topic = calloc(count, sizeof(char*));
    if (!tokenized_topic)
        err(1, "tokenize topic calloc tokenized_topic");
    temp = strtok(topic, "/");
    for (int i = 0; i < count; i++) {
        tokenized_topic[i] = calloc(strlen(temp), sizeof(char));
        strncpy(tokenized_topic[i], temp, strlen(temp));
        temp = strtok(NULL, "/");
    }

    *count_out = count;

    return tokenized_topic;
}

void
insert_topic(topics_t *list, char *topic_str, int qos_code) {
    topic_t *topic = calloc(1, sizeof(topic_t));
    if (!topic)
        err(1, "insert topic calloc topic");
    topic_t *head = list->head;

    if (head == NULL) {
        head = topic;
    }
    else {
        head->next = topic;
    }
    topic->next = NULL;
    char* topic_copy = calloc(strlen(topic_str), sizeof(char));
    if (!topic_copy)
        err(1, "insert topic calloc topic_copy");
    strcpy(topic_copy, topic_str);
    topic->topic = topic_copy;
    topic->tokenized_topic = tokenize_topic(topic_str, &topic->topic_token_count);
    topic->qos_code = qos_code;

    if (list->back == NULL) {
        list->back = topic;
    }

    list->head = topic;
}

int
remove_topic(topics_t *list, char *topic_str) {
    topic_t *prev_topic = NULL;
    for (topic_t *topic = list->back; topic != NULL; topic = topic->next) {
        if (strcmp(topic->topic, topic_str) == 0) {
            free(topic->topic);
            // for (int i = 0; topic->topic_token_count; i++) {
            //     free(topic->tokenized_topic + i);
            // }
            if (prev_topic != NULL) {
                prev_topic->next = topic->next;
                if (topic == list->head)
                    list->head = prev_topic;
            }
            else {
                list->back = topic->next;
                if (topic == list->head)
                    list->head = NULL;
            }
            free(topic);
            return 1;
        }

        prev_topic = topic;
    }
    return 0;
}

int
find_topic(topics_t *list, char *topic_str) {
    for (topic_t *topic = list->back; topic != NULL; topic = topic->next) {
        if (strcmp(topic->topic, topic_str) == 0)
            return 1;
    }
    return 0;
}

int
insert_topic_checked(topics_t *list, char *topic_str, int qos_code) {
    if (find_topic(list, topic_str))
        return 0;
    else {
        insert_topic(list, topic_str, qos_code);
        return 1;
    }
}

topics_t *
create_topics_list() {
    topics_t *list = calloc(1, sizeof(topics_t));
    if (!list)
        err(1, "create topic list calloc list");
    list->back = NULL;
    list->head = NULL;
    return list;
}

void
delete_topics_list(topics_t *list) {
    topic_t *prev_topic = NULL;

    for (topic_t *topic = list->back; topic != NULL; topic = topic->next) {
        free(topic->topic);
        if (prev_topic)
            free(prev_topic);
        prev_topic = topic;
    }

    if (prev_topic)
        free(prev_topic);

    free(list);
}
