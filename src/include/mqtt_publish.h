#ifndef FEMTO_MQTT_MSG_PUBLISH_H
#define FEMTO_MQTT_MSG_PUBLISH_H

#include "mqtt_utils.h"

struct publish {
    char *topic;
    char *message;
    uint32_t message_size;
    uint16_t topic_size;
};

typedef struct publish publish_t;

publish_t *
read_publish_message(conn_t *conn, char *incoming_message);

void
send_published_message(conns_t *conns, publish_t *publish);

#endif
