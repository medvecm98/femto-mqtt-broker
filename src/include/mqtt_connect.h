#ifndef FEMTO_MQTT_MSG_CONNECT_H
#define FEMTO_MQTT_MSG_CONNECT_H

#include "mqtt_utils.h"
#include <ctype.h>

int
check_protocol_name(char *index);

int
check_protocol_level(char *index);

int
get_keep_alive(char *index);

char*
get_client_id(char *index);

char *
create_connack_message(uint8_t return_code);

int
read_connect_message(conns_t *conns, conn_t *conn, char* incoming_message);

char *
create_connect_response(conn_t *conn, conns_t *conns, int code);

#endif
