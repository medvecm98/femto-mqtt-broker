#include "mqtt_publish.h"

publish_t *
read_publish_message(conn_t *conn, char *incoming_message) {
    char *index = incoming_message;
    publish_t *publish = calloc(1, sizeof(publish_t));

    uint16_t topic_name_len = 0;
    topic_name_len = (uint8_t)index[0] << 8;
    topic_name_len |= index[1];

    publish->topic = calloc(topic_name_len + 1, sizeof(char));
    strncpy(publish->topic, index + 2, topic_name_len);

    uint32_t msg_len = conn->message_size - 2 - topic_name_len;
    publish->message = calloc(msg_len + 1, sizeof(char));
    strncpy(publish->message, index + 2 + topic_name_len, msg_len);

    publish->message_size = msg_len;
    publish->topic_size = topic_name_len;

    return publish;
}

int
topic_match(topic_t *topic_subbed, char *topic_published) {
    if (
        topic_published[0] == '$' && 
        (
            strcmp(topic_subbed->tokenized_topic[0], "#") == 0 ||
            strcmp(topic_subbed->tokenized_topic[0], "+") == 0
        )
    ) {
        return 0;
    }

    int count = 0;
    for (
        char *token = strtok(topic_published, "/");
        token != NULL;
        token = strtok(NULL, "/"), count++
    ) {
        if (count > topic_subbed->topic_token_count)
            return 0;
        if (strcmp(topic_subbed->tokenized_topic[count], "+") == 0)
            continue;
        else if (strcmp(topic_subbed->tokenized_topic[count], token) == 0) {
            continue;
        }
        else if (strcmp(topic_subbed->tokenized_topic[count], "#") == 0)
            return 1;
        else
            return 0;
    }

    if (
        (count == topic_subbed->topic_token_count - 1) &&
        strcmp(topic_subbed->tokenized_topic[count], "#") == 0
    ) {
        return 1;
    }

    if (count != topic_subbed->topic_token_count) {
        return 0;
    }

    return 1;
}

void
create_publish_message(conn_t *conn, publish_t *publish) {
    char *rem_len = from_uint_to_val_len(2 + publish->topic_size + publish->message_size);
    int rem_len_len = strlen(rem_len);
    int len = 0;

    char* message = calloc(1 + rem_len_len + 2 + publish->topic_size + publish->message_size, sizeof(char));
    char* message_orig = message;

    // control packet type
    *message = 0x30;
    message++;
    len++;

    // remaining length
    strncpy(message, rem_len, rem_len_len);
    message += rem_len_len;
    len += rem_len_len;

    // topic (16 bit size)
    *message = (publish->topic_size >> 8) & 0x00FF;
    message++;
    *message |= publish->topic_size & 0x00FF;
    message++;
    len += 2;

    // topic (string)
    strncpy(message, publish->topic, publish->topic_size);
    log_trace("Topic: %s", publish->topic);
    log_trace("Topic size: %d", publish->topic_size);
    message += publish->topic_size;
    len += publish->topic_size;

    // message
    memcpy(message, publish->message, publish->message_size);
    len += publish->message_size;

    // mark for export
    conn->state = 1;
    conn->message = message_orig;
    conn->message_size = len;
}

void
send_published_message(conns_t *conns, publish_t *publish) {
    for (
        conn_t *conn = conns->conn_back;
        conn != NULL;
        conn = conn->next
    ) {
        for (
            topic_t *topic = conn->topics->back;
            topic != NULL;
            topic = topic->next
        ) {
            char *publish_topic_copy = calloc(publish->topic_size, sizeof(char));
            strcpy(publish_topic_copy, publish->topic);
            if (topic_match(topic, publish_topic_copy)) {
                create_publish_message(conn, publish);
            }
            free(publish_topic_copy);
        }
    }
}
