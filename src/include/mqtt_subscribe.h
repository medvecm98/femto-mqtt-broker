#ifndef FEMTO_MQTT_MSG_SUBSCRIBE_H
#define FEMTO_MQTT_MSG_SUBSCRIBE_H

#include "mqtt_utils.h"
#include "poll_list.h"

/**
 * Read, parse incoming (UN)SUBSCRIBE MQTT control packet.
 * 
 * \returns Number of topics found.
 */
int
read_un_subscribe_message(conn_t *conn, char *incoming_message);

/**
 * Create response to (UN)SUBSCRIBE MQTT control packet.
 * 
 * \param topics_inserted_code How many topics were inserted, or error code, if
 * number is negative.
 * 
 * \returns (UN)SUBACK MQTT control packet, in bytes.
 */
char *
create_un_subscribe_response(
    conn_t *conn, conns_t *conns, int topics_inserted_code
);

#endif
