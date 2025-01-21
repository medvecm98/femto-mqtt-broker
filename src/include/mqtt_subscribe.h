#ifndef FEMTO_MQTT_MSG_SUBSCRIBE_H
#define FEMTO_MQTT_MSG_SUBSCRIBE_H

#include "mqtt_utils.h"


int
read_un_subscribe_message(conn_t *conn, char *incoming_message);

char *
create_un_subscribe_response(conn_t *conn, conns_t *conns, int topics_inserted_code);

#endif
