#ifndef FEMTO_MQTT_MSG_PUBLISH_H
#define FEMTO_MQTT_MSG_PUBLISH_H

#include "mqtt_utils.h"

/**
 * Struct used for wrapping information about publishing a message.
 */
typedef struct {
    char *topic;
    char *message;
    uint32_t message_size; // size in bytes
    uint16_t topic_size; // size in bytes
} publish_t;

/**
 * Read, parse incoming PUBLISH MQTT control packet.
 * 
 * \returns publish_t struct containing publish info.
 */
publish_t *
read_publish_message(conn_t *conn, char *incoming_message);

/**
 * Finds out which clients are subscribet to given topic in published message
 * and sends them the message.
 * 
 * Message size, its contents, status and type are set accordingly.
 * 
 * \param sender_conn Connection to the sender (publisher).
 * \param conns Connections linked list.
 * \param publish Information about publish.
 * 
 * \returns 1, if message is published to publisher as well, 0 otherwise.
 */
int
send_published_message(conn_t *sender_conn, conns_t *conns, publish_t *publish);

#endif
