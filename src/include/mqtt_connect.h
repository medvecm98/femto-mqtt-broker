#ifndef FEMTO_MQTT_MSG_CONNECT_H
#define FEMTO_MQTT_MSG_CONNECT_H

#include "mqtt_utils.h"
#include <ctype.h>

/**
 * Read, parse incoming CONNECT MQTT control packet.
 * 
 * Errors: 1 - invalid protocol name. 2 - invalid protocol level.
 * 3 - invalid client name.
 * 
 * \param conns Connections linked list.
 * \param conn Connection struct of connectee.
 * \param incoming_message MQTT variable header and payload in byte form.
 * 
 * \returns Zero if success or error code.
 */
int
read_connect_message(conns_t *conns, conn_t *conn, char* incoming_message);

/**
 * Creates CONNACK MQTT control packet based on provided code.
 * 
 * \returns CONNACK MQTT control packet in bytes form.
 */
char *
create_connect_response(conn_t *conn, conns_t *conns, int code);

#endif
