#include "mqtt_subscribe.h"

uint16_t
get_packet_id(char* index) {
    uint8_t *index8 = (uint8_t*) index;
    uint16_t packet_id = *index8;
    packet_id <<= 8;
    packet_id |= *(index8 + 1);
    return packet_id;
}

int
read_variable_header(conn_t *conn, char *incoming_message) {
    char *index = incoming_message;

    conn->packet_id = get_packet_id(index);
    index += 2;

    return 0;
}

int
read_payload(conn_t *conn, char *incoming_message) {
    char *index = incoming_message;
    int rem_len = conn->message_size;

    rem_len -= 2; // for variable header

    if (rem_len == 0) {
        conn->delete_me = 1;
        return -1;
    }

    int length = 0;
    int topic_counter = 0;
    char *topic;
    while (rem_len > 0) {
        length = index[0];
        length <<= 8;
        length |= index[1];
        index += 2;
        rem_len -= 2;

        topic = calloc(length, 1);
        for (int i = 0; i < length; i++) {
            topic[i] = *index;
            index++;
            rem_len--;
        }

        //skip qos
        index++;
        rem_len--;

        if (conn->type == MQTT_SUBSCRIBE) {
            insert_topic(conn->topics, topic, 0x00);
            free(topic);
        }
        else {
            remove_topic(conn->topics, topic);
            free(topic);
        }

        topic_counter++;
    }

    return topic_counter;
}

int
read_un_subscribe_message(conn_t *conn, char *incoming_message) {
    char *index = incoming_message;

    (void) read_variable_header(conn, index);
    index += 2;

    int topic_counter = read_payload(conn, index);
    return topic_counter;
}

char *
create_suback_message(conn_t *conn, int topic_counter) {
    char *buffer = calloc(4 + topic_counter, sizeof(char));
    char *buffer_orig = buffer;
    int len = 0;

    buffer[0] = (char) 0x90;
    len++;

    // get rem len in variable length format
    char *rem_len = from_uint_to_val_len(2 + topic_counter);
    size_t rem_len_len = strlen(rem_len);

    // write remaining length
    for (int i = 0; i < rem_len_len; i++) {
        buffer[1 + i] = rem_len[i];
        len++;
    }
    free(rem_len);

    // advance for "remaininig length" length and first byte (control id...)
    buffer += 1 + rem_len_len;

    // write packet id
    *buffer = (conn->packet_id >> 8) & 0x00FF;
    buffer++;
    *buffer = conn->packet_id & 0x00FF;
    buffer++;
    len += 2;

    // write topic answers
    topic_t *topic_iter = NULL;
    if (conn->last_topic_before_insert)
        topic_iter = conn->last_topic_before_insert->next;
    else 
        topic_iter = conn->topics->head;

    for (int i = 0; i < topic_counter; i++) {
        *buffer = topic_iter->qos_code;
        topic_iter = topic_iter->next;
        len++;
    }

    conn->message_size = len;

    return buffer_orig;
}

char *
create_unsuback_message(conn_t *conn) {
    char* buffer = calloc(4, sizeof(char));

    buffer[0] = (char) 0xB0;
    buffer[1] = 0x02;
    buffer[2] = (conn->packet_id >> 8) & 0x00FF;
    buffer[3] = conn->packet_id & 0x00FF;

    conn->message_size = 4;

    return buffer;
}

char *
create_un_subscribe_response(conn_t *conn, conns_t *conns, int topics_inserted_code) {
    if (topics_inserted_code == -1) {
        conn->delete_me = 1;
        return NULL;
    }

    if (conn->type == MQTT_SUBSCRIBE)
        return create_suback_message(conn, topics_inserted_code);
    else
        return create_unsuback_message(conn);
}
