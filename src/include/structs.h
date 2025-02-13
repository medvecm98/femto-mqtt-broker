#ifndef FEMTO_MQTT_STRUCTS_H
#define FEMTO_MQTT_STRUCTS_H

#include <poll.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <err.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include "log.h"
#include "topic_list.h"

/**
 * All MQTT control packet types. Values are the same as in upper 4 bits in
 * MQTT control packet first byte [type|flags].
 */
enum mqtt_control_packet_type {
	MQTT_CONNECT = 1,
	MQTT_CONNACK = 2,
	MQTT_PUBLISH = 3,
	MQTT_PUBACK = 4,
	MQTT_SUBSCRIBE = 8,
	MQTT_SUBACK = 9,
	MQTT_UNSUBSCRIBE = 10,
	MQTT_UNSUBACK = 11,
	MQTT_PINGREQ = 12,
	MQTT_PINGRESP = 13,
	MQTT_DISCONNECT = 14
};

/**
 * Type for MQTT control packet type.
 * 
 * All MQTT control packet types. Values are the same as in upper 4 bits in
 * MQTT control packet first byte [type|flags].
 */
typedef enum mqtt_control_packet_type ctrl_packet_t;

/**
 * Connection struct containing all information related to connected clients.
 * Mainly contains their in-/out-bound messages, keep alive value, topics they
 * are subscribed to, struct pollfd for `poll`, etc.
 * 
 * Also it's an entry in connections linked list and so pointers to next and
 * previous list entry are neccessary.
 */
struct connection {
	struct pollfd pfd; // for poll syscall, containing client's socked fd

	struct connection *next; // next entry in connections linked list
	struct connection *prev; // previous entry in connections linked list

	topics_t *topics; // subscribed topics linked list
	char *client_id;

	char *message; // message in bytes form (no \0)
	ssize_t message_size; // message size in bytes (no \0)
	ctrl_packet_t type;
	int keep_alive;
    uint16_t packet_id;
	/* last topic in topic list before insertion of new one */
    topic_t *last_topic_before_insert;

	int delete_me; // client will be disconnected and deleted
	int64_t last_seen; // last time this client sent some control packet
	uint8_t seen_connect_packet; // we can't see two connect ctrl packets

	int state; // 0 - incoming, 1 - outgoing
};

typedef struct connection conn_t;

/**
 * Connections linked list, containing all connected clients to this MQTT broker
 */
struct connections {
	struct connection *conn_head;
	struct connection *conn_back;
	int count;
};

typedef struct connections conns_t;

#endif
