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

#define SIZE 256
#define READ_SIZE 256
#define RCVD_SIZE_DEFAULT 4096
#define OUTGOING_SIZE_DEFAULT 4096
#define POLL_WAIT_TIME 150

/**
 * NOTES:
 * - Fixed header is 2 to 5 bytes.
 * - Rest is up to 256 MiB.
 * - read buffer about 4 KiB
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

typedef enum mqtt_control_packet_type ctrl_packet_t;

struct connection {
	struct pollfd pfd;

	struct connection *next;
	struct connection *prev;

	char *topic;

	char *message;
	int message_allocd_len;
	ssize_t message_size;
	ctrl_packet_t type;

	int delete_me;
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


struct mqtt_message {
	ctrl_packet_t type;
};

void
clear_pending_message(struct connection *conn, int should_free) {
	if (should_free) {
		free(conn->message);
	}

	conn->message_allocd_len = RCVD_SIZE_DEFAULT;
	conn->message = calloc(conn->message_allocd_len, sizeof(char));
	conn->message_size = 0;
	conn->state = 0;
}

void
add_connection(struct connections *conns, int fd) {
	struct connection *new_connection = calloc(1, sizeof(struct connection));

	if (!new_connection)
		err(1, "calloc");

	new_connection->next = NULL;
	new_connection->prev = NULL;
	// new_connection->next_topic_index = 0;
	// memset(&new_connection->pfd, 0, sizeof(struct pollfd));

	// set 'back' if it is NULL
	if (!conns->conn_back) {
		conns->conn_back = new_connection;
	}

	// set prev/next
	if (conns->conn_head) {
		// we already had a head
		new_connection->prev = conns->conn_head;
		conns->conn_head->next = new_connection;
	}

	// set new head
	conns->conn_head = new_connection;

	conns->count++;

	new_connection->pfd.fd = fd;
	new_connection->pfd.events = POLLIN | POLLOUT;
	new_connection->delete_me = 0;
	clear_pending_message(new_connection, 0);
}

void
sub_topic(struct connection *conn, char *topic) {
	conn->topic = topic;
}

void
unsub_topic(struct connection *conn, char *topic) {
	if (strcmp(conn->topic, topic) == 0) {
		conn->topic = NULL;
	}
	else {
		warn("topic to remove didn't match");
	}
}

int
find_connection(char* portstr) {
	int sock_fd = 0;

	struct addrinfo *info, *info_orig, hint;
	memset(&hint, 0, sizeof(hint));

	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_UNSPEC;
	hint.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, portstr, &hint, &info_orig) != 0)
		err(2, "getaddrinfo");

	int bind_successful = 0;
	for (info = info_orig; info != NULL; info = info->ai_next) {
		sock_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		int opt = 1;
		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			err(1, "setsockopt (for main listening socket)");

		if (sock_fd == -1)
			err(1, "socket");
		if (!bind(sock_fd, info->ai_addr, info->ai_addrlen)) {
			bind_successful = 1;
			break;
		}
	}

	if (!bind_successful) {
		err(1, "Did not find any available address");
	}

	freeaddrinfo(info_orig);

	return sock_fd;
}

void
conns_init(struct connections *conns) {
	memset(conns, 0, sizeof(struct connections));
	conns->count = 0;
	conns->conn_back = NULL;
	conns->conn_head = NULL;
}

ctrl_packet_t
get_mqtt_type(char packet_type_flags) {
	int type = packet_type_flags;
	type >>= 4;
	return type;
}

int
from_val_len_to_uint(char *buffer) {
	int multiplier = 1;
	int value = 0;
	char encoded_byte = 0;
	int index = 0;

	do {
		encoded_byte = buffer[index++];
		value += (encoded_byte & 127) * multiplier;
		multiplier *= 128;
		if (multiplier > 128 * 128 * 128) {
			log_error("Malformed remaining length.");
			return -1;
		}
	} while (encoded_byte & 128 != 0);

	return value;
}

void
process_incoming_data_from_client(struct connection *conn) {
	char packet_type_flags = 0;
	char *buffer = calloc(4, sizeof(char));
	int fd = conn->pfd.fd;

	if (read(fd, &packet_type_flags, 1) == -1) {
		log_error("Failed to read type and flags.");
		conn->delete_me = 1;
	}

	conn->type = get_mqtt_type(packet_type_flags);

	// read first byte of remaining length
	if (read(fd, buffer, 1) == -1)
		err(1, "read 1st 2nd bytes");

	int rem_len_last_byte = 0;
	if (buffer[rem_len_last_byte] & 128) {
		if (rem_len_last_byte == 4) {
			log_error("Remaining length is way too long.");
			conn->delete_me = 1;
		}

		// there is more to remaining length
		if (read(fd, buffer + rem_len_last_byte, 1) == -1)
			err(1, "rem length read fail");

		rem_len_last_byte++;
	}
	
	int rem_len = from_val_len_to_uint(buffer);

	free(buffer);
	buffer = calloc(rem_len, sizeof(char));
	ssize_t bytes_read = read(fd, buffer, rem_len);

	if (bytes_read == -1)
		err(1, "read");
	else if (bytes_read != rem_len) {
		log_error("Message read and remaining length values don't match.");
		conn->delete_me = 1;
	}
	else if (bytes_read == 0) {
		conn->delete_me = 1;
	}
	else {
		conn->message_size = rem_len;
		conn->message = buffer;
	}
}

void
print_conns(struct connections* conns) {
	if (conns->count == 0) {
		printf("No connections were found.\n");
		return;
	}
	printf("Printing connections:\n");
	int i = 0;
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		printf("    [%d] fd: %d\n", i++, conn->pfd.fd);
	}
}

void
print_message(struct connection* conn) {
	for (int i = 0; i < conn->message_size; i++) {
		printf("%02x ", conn->message[i]);
	}
	printf("\n");
}

/**
 * Checks for POLLIN `poll` flag in active connections.
 * 
 * \param conns pointer to connections structure
 */
void
check_poll_in(struct connections *conns) {
	if (conns->count == 0) return;

	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (poll(&conn->pfd, 1, POLL_WAIT_TIME) == -1)
			err(1, "poll in check");

		if (conn->pfd.revents & POLLIN) {
			log_debug("Found POLLIN\n");
			process_incoming_data_from_client(conn);
			print_message(conn);
		}
	}
}

void
poll_and_accept(struct connections *conns, struct pollfd *listening_pfd) {
	int nfd = -1;
	listening_pfd->events = POLLIN;

	if (poll(listening_pfd, 1, POLL_WAIT_TIME) == -1)
		err(1, "poll listening pfd");

	if (listening_pfd->revents & POLLIN) {
		if ((nfd = accept(listening_pfd->fd, NULL, NULL)) == -1)
			err(3, "accept");
		log_debug("accepted\n");
		add_connection(conns, nfd);
	}
}

void
check_poll_hup(struct connections *conns) {
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (poll(&conn->pfd, 1, POLL_WAIT_TIME) == -1)
			err(1, "poll hup check");

		if (conn->pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
			log_debug("Found POLLHUP\n");
			conn->delete_me = 1;
		}
	}
}

void
check_poll_out(struct connections *conns) {
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (poll(&conn->pfd, 1, POLL_WAIT_TIME) == -1)
			err(1, "poll out check");

		if (conn->state == 1 && conn->message_size > 0 && conn->pfd.revents & POLLOUT) {
			log_debug("Found POLLOUT\n");
			write(conn->pfd.fd, conn->message, conn->message_size);
			clear_pending_message(conn, 1);
		}
	}
}

void
print_index(char *index) {
	printf("index: %#04x\n", *index);
}

int
check_protocol_name(char *index) {
	if (*index != 0)
		return 0;
	index++;
	
	if (*index != 4)
		return 0;
	index++;
	
	if (*index != 'M')
		return 0;
	index++;
	
	if (*index != 'Q')
		return 0;
	index++;

	if (*index != 'T')
		return 0;
	index++;

	if (*index != 'T')
		return 0;
	index++;
	
	return 1;
}

int
check_protocol_level(char *index) {
	if (*index == 4) {
		return 1;
	}
	else
		return 0;
}

int
get_keep_alive(char *index) {
	int keep_alive = *index;
	keep_alive <<= 1;
	keep_alive = keep_alive | *(index + 1);
	index += 2;

	return keep_alive;
}

char*
get_client_id(char *index) {
	uint16_t client_id_length = *index;
	index++;
	client_id_length <<= 1;
	client_id_length |= *index;
	index++;
	
	char *client_id = calloc(client_id_length + 1, 1);
	strncpy(client_id, index, client_id_length);

	return client_id;
}

ssize_t
handle_connect_message(char* incoming_message, char* outgoing_message) {
	char *index = incoming_message;

	if (!check_protocol_name(index)) {
		log_error("Invalid protocol name in CONNECT variable header.");
		return -1;
	}
	index += 6;

	if (!check_protocol_level(index)) {
		log_error("Invalid protocol level in CONNECT variable header.");
		return -1;
	}
	index += 2;

	// skip flags
	index += 1;

	int keep_alive = get_keep_alive(index);

	char *client_id = get_client_id(index);
	printf("client id: %s\n", client_id);

	return 0;
}

void
process_mqtt_message(struct connection *conn) {
	if (conn->message_size > 0) {
		char *incoming_message, *outgoing_message;
		incoming_message = conn->message; // no fixed header here
		outgoing_message = calloc(OUTGOING_SIZE_DEFAULT, sizeof(char));

		ssize_t remaining_length = 0;
		switch (conn->type) {
			case MQTT_CONNECT:
				handle_connect_message(incoming_message, outgoing_message);
				break;
			default:
				log_error("Not implemented / unsupported.");
				conn->delete_me = 1;
				break;
		}

		free(conn->message);
		conn->message = outgoing_message;
		conn->state = 1;
	}
}

void
check_and_process_mqtt_messages(struct connections *conns) {
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (conn->message_size > 0) {
			process_mqtt_message(conn);
		}
	}
}

void
clear_connections(struct connections *conns) {
	struct connection *prev, *next;
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = next
	) {
		next = conn->next;
		if (conn->delete_me) {
			prev = conn->prev;

			if (prev)
				prev->next = next;

			if (next)
				next->prev = prev;
			
			if (conn == conns->conn_back) {
				conns->conn_back = next;
			}

			if (conn == conns->conn_head) {
				conns->conn_head = prev;
			}

			conns->count--;
			free(conn->message);

			free(conn);
		}
	}
}

int
main(int argc, char* argv[]) {
	char* portstr = "1883";
	int nclients = SIZE;

	int sock_fd = find_connection(portstr);
	struct connections conns;
	conns_init(&conns);

	if (listen(sock_fd, nclients) == -1)
		err(3, "listen");

	struct pollfd listening_pfd;
	listening_pfd.fd = sock_fd;
	for (;;) {
		poll_and_accept(&conns, &listening_pfd);
		print_conns(&conns);

		check_poll_hup(&conns);
		clear_connections(&conns);
		check_poll_in(&conns);
		check_and_process_mqtt_messages(&conns);
		check_poll_out(&conns);
	}

	return 0;
}
