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

typedef enum mqtt_control_packet_type ctrl_packet_t;

struct connection {
	struct pollfd pfd;

	struct connection *next;
	struct connection *prev;

	topics_t *topics;
	char *client_id;

	char *message;
	int message_allocd_len;
	ssize_t message_size;
	ctrl_packet_t type;
	int keep_alive;
    uint16_t packet_id;
    topic_t *last_topic_before_insert;

	int delete_me;
	int skip_me;
	int64_t last_seen;
	uint8_t seen_connect_packet;
	// int connection_accepted;
	// int next_topic_index;

	int state; // 0 - incoming, 1 - outgoing
};

typedef struct connection conn_t;

struct connections {
	struct connection *conn_head;
	struct connection *conn_back;
	int count;
};

typedef struct connections conns_t;

struct mqtt_message {
	ctrl_packet_t type;
};

#endif
