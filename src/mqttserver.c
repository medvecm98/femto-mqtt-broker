#define _GNU_SOURCE

#include "mqtt_connect.h"
#include "mqtt_subscribe.h"
#include "mqtt_publish.h"
#include <signal.h>
#include <time.h>

#define SIZE 256
#define READ_SIZE 256
#define RCVD_SIZE_DEFAULT 4096
#define OUTGOING_SIZE_DEFAULT 4096
#define POLL_WAIT_TIME 20

static conns_t *global_conns = NULL;

/**
 * NOTES:
 * - Fixed header is 2 to 5 bytes.
 * - Rest is up to 256 MiB.
 * - read buffer about 4 KiB
 */


void
clear_message(struct connection *conn, int should_free) {
	if (should_free) {
		free(conn->message);
	}

	conn->message_size = 0;
	conn->state = 0;
	conn->last_topic_before_insert = NULL;
	conn->packet_id = 0;
}

void
add_connection(struct connections *conns, int fd) {
	struct connection *new_connection = calloc(1, sizeof(struct connection));

	if (!new_connection)
		err(1, "calloc add connection");

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
	new_connection->skip_me = 0;
	new_connection->client_id = NULL;
	new_connection->keep_alive = 0;
	new_connection->topics = create_topics_list();
	new_connection->packet_id = 0;
	new_connection->last_topic_before_insert = NULL;
	new_connection->last_seen = time(NULL);
	new_connection->seen_connect_packet = 0;

	clear_message(new_connection, 0);
}

int
find_connection(char* portstr) {
	int sock_fd = 0;

	struct addrinfo *info, *info_orig, hint;
	memset(&hint, 0, sizeof(hint));

	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET6;
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
get_mqtt_type(uint8_t packet_type_flags) {
	unsigned int type = packet_type_flags & 0xF0;
	type >>= 4;
	return (ctrl_packet_t) type;
}

uint8_t
check_subscribe_flags(uint8_t packet_type_flags) {
	if ((packet_type_flags & 0x0F) != 0x02)
		return 0;
	else
		return 1;
}

uint8_t
check_zeroed_flags(uint8_t packet_type_flags) {
	if ((packet_type_flags & 0x0F) != 0x00)
		return 0;
	else
		return 1;
}

void
process_incoming_data_from_client(struct connection *conn, char *fixed_header) {
	char packet_type_flags = fixed_header[0];
	int fd = conn->pfd.fd;

	conn->type = get_mqtt_type((uint8_t) packet_type_flags);
	// log_debug("Type received: %d", conn->type);

	if (conn->type == MQTT_PINGREQ || conn->type == MQTT_DISCONNECT) {
		// for messages with empty remaining length
		conn->message_size = -1;
		return;
	}

	if (conn->type == MQTT_SUBSCRIBE || conn->type == MQTT_UNSUBSCRIBE) {
		if (!check_subscribe_flags(packet_type_flags)) {
			log_warn("Invalid flags for (UN)SUBSCRIBE control packet.");
			conn->delete_me = 1;
		}
	}
	else {
		if (!check_zeroed_flags(packet_type_flags)) {
			log_warn("Invalid flags for control packet (first byte of fixed header).");
			conn->delete_me = 1;
		}
	}

	int rem_len = from_val_len_to_uint(fixed_header + 1);

	char *buffer = calloc(rem_len, sizeof(char));
	if (!buffer)
		err(1, "process incoming data calloc buffer reallocate");

	ssize_t bytes_read = read(fd, buffer, rem_len);
	ssize_t bytes_read_acc = bytes_read;
	while (bytes_read_acc < rem_len && bytes_read > 0) {
		bytes_read = read(fd, buffer + bytes_read_acc, rem_len - bytes_read_acc);
		if (bytes_read == -1)
			err(1, "process incoming data read");
		bytes_read_acc += bytes_read;
	}

	if (bytes_read == -1)
		err(1, "read");
	else if (bytes_read_acc != rem_len) {
		log_error("Message read and remaining length values don't match.");
		conn->delete_me = 1;
		// free(buffer);
	}
	else if (bytes_read == 0) {
		log_error("No bytes were read.");
		conn->delete_me = 1;
		// free(buffer);
	}
	else {
		conn->message_size = rem_len;
		conn->message = buffer;
	}
}

void
print_conns(struct connections* conns) {
	if (conns->count == 0) {
		// log_debug("No connections were found.");
		return;
	}
	log_debug("Printing connections:");
	int i = 0, j;
	for (
		struct connection *conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		log_debug(".%d. fd: %p", i, conn);
		log_debug(".%d. fd: %d", i, conn->pfd.fd);
		log_debug(".%d. client_id: %s", i, conn->client_id);
		log_debug(".%d. keep_alive: %d", i, conn->keep_alive);
		log_debug(".%d. message size: %d", i, conn->message_size);
		log_debug(".%d. topics:", i);
		j = 0;
		for (
			topic_t *topic = conn->topics->back;
			topic != NULL;
			topic = topic->next
		) {
			log_debug(".%d.%d. topic: %s", i, j, topic->topic);
			for (int k = 0; k < topic->topic_token_count; k++) {
				log_debug(".%d.%d.%d. token: %s", i, j, k, topic->tokenized_topic[k]);
			}
			log_debug(".%d.%d. QoS code: %#04x", i, j, topic->qos_code);
			j++;
		}
		i++;
	}
}

void
print_message(struct connection* conn) {
	char* s = "M: ";
	for (int i = 0; i < conn->message_size; i++) {
	//	asprintf(&s, "%s%02x ", s, conn->message[i]);
	}
	log_debug(s);
}

char*
read_fixed_header(conn_t *conn) {
	char *buffer = calloc(6, 1);
	if (!buffer)
		err(1, "fixed header calloc buffer");
	ssize_t read_bytes = 0;
	ssize_t read_bytes_acc = 0;

	read_bytes = read(conn->pfd.fd, buffer, 2);
	read_bytes_acc += read_bytes;
	// log_debug("read bytes: %d, acc: %d", read_bytes, read_bytes_acc);

	if (read_bytes == 0) {
		// conn->delete_me = 1;
		conn->skip_me = 1;
		return NULL;
	}

	// read packet control type and first remaining length byte
	while (read_bytes_acc < 2) {
		read_bytes = read(conn->pfd.fd, buffer + read_bytes_acc, 2 - read_bytes_acc);
		read_bytes_acc += read_bytes;
	}

	// read other remaining length bytes, if needed
	while ((buffer[read_bytes_acc - 1] & 128) != 0 && read_bytes_acc < 5) {
		read_bytes = read(conn->pfd.fd, buffer + read_bytes_acc, 1);
		read_bytes_acc += read_bytes;
	}

	// check if remaining len isn't too long
	if ((buffer[read_bytes_acc - 1] & 128) != 0 && read_bytes_acc >= 5) {
		conn->delete_me = 1;
		return NULL;
	}

	return buffer;
}

void
print_hex(char *buffer) {
	char* s = "Hex: ";
	for (int i = 0; i < strlen(buffer); i++) {
	//	asprintf(&s, "%s%02x ", s, buffer[i]);
	}
	log_debug(s);
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
		if (conn->skip_me) continue;

		if (poll(&conn->pfd, 1, POLL_WAIT_TIME) == -1)
			err(1, "poll in check");

		if (conn->pfd.revents & POLLIN) {
			log_debug("Found POLLIN");
			char *fixed_header = read_fixed_header(conn);
			if (!fixed_header && !conn->delete_me) {
				continue;
			}
			// print_hex(fixed_header);
			process_incoming_data_from_client(conn, fixed_header);
			// print_message(conn);
			free(fixed_header);
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
		log_debug("accepted");
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
		if (conn->skip_me) continue;
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
		if (conn->skip_me) continue;
		if (poll(&conn->pfd, 1, POLL_WAIT_TIME) == -1)
			err(1, "poll out check");

		if (conn->state == 1 && conn->message_size > 0 && conn->pfd.revents & POLLOUT) {
			// log_debug("Found POLLOUT");
			if (conn->type == MQTT_PUBLISH)
				log_trace("publish...");
			write(conn->pfd.fd, conn->message, conn->message_size);
			clear_message(conn, 1);
		}
	}
}

void
print_index(char *index) {
	log_trace("index: %#04x\n", *index);
}

void
process_mqtt_message(struct connection *conn, struct connections *conns) {
	char *incoming_message, *outgoing_message;
	incoming_message = conn->message; // no fixed header here
	outgoing_message = NULL;
	int code = 255;
	int topics_inserted_code = 255;
	conn->last_seen = time(NULL);

	if (conn->type != MQTT_CONNECT && conn->seen_connect_packet == 0) {
		conn->delete_me = 1;
		// print_conns(conns);
		return;
	}

	int publish_send_to_sender = 0;
	switch (conn->type) {
		case MQTT_CONNECT:
			log_debug("Received CONNECT");
			if (conn->seen_connect_packet == 1) {
				conn->delete_me = 1;
				print_conns(conns);
				return;
			}
			conn->seen_connect_packet = 1;
			code = read_connect_message(conns, conn, incoming_message);
			outgoing_message = create_connect_response(conn, conns, code);
			break;
		case MQTT_DISCONNECT:
			log_info("Client %s disconnecting.", conn->client_id);
			conn->delete_me = 1;
			break;
		case MQTT_SUBSCRIBE:
			log_debug("Received SUBSCRIBE from %s", conn->client_id);
			conn->last_topic_before_insert = conn->topics->head;
			topics_inserted_code = read_un_subscribe_message(conn, incoming_message);
			outgoing_message = create_un_subscribe_response(conn, conns, topics_inserted_code);
			print_conns(conns);
			break;
		case MQTT_UNSUBSCRIBE:
			log_debug("Received UNSUBSCRIBE from %s", conn->client_id);
			conn->last_topic_before_insert = conn->topics->head;
			topics_inserted_code = read_un_subscribe_message(conn, incoming_message);
			outgoing_message = create_un_subscribe_response(conn, conns, topics_inserted_code);
			print_conns(conns);
			break;
		case MQTT_PUBLISH:
			;
			log_debug("Received PUBLISH from %s", conn->client_id);

			publish_t *publish = read_publish_message(conn, incoming_message);
			publish_send_to_sender = send_published_message(conn, conns, publish);

			free(publish->topic);
			free(publish->message);
			free(publish);
			break;
		case MQTT_PINGREQ:
			log_debug("Received ping from %s", conn->client_id);
			outgoing_message = calloc(2, 1);
			if (!outgoing_message)
				err(1, "process mqtt msg calloc outgoing_message");
			*outgoing_message = (char) 0xD0;
			*(outgoing_message + 1) = 0x00;
			conn->message_size = 2;
			break;
		default:
			log_error("Not implemented / unsupported from %s", conn->client_id);
			conn->delete_me = 1;
			break;
	}

	if (conn->type != MQTT_PUBLISH && conn->type != MQTT_DISCONNECT) {
		if (conn->type != MQTT_PINGREQ)
			free(conn->message);
		// no answer to sender in PUBLISH message (in QoS 0)
		conn->message = outgoing_message;
		conn->state = 1;
	}

	if (conn->type == MQTT_PUBLISH && !publish_send_to_sender) {
		clear_message(conn, 1);
	}

	// print_conns(conns);
}

void
check_and_process_mqtt_messages(struct connections *conns) {
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (conn->message_size != 0 && conn->state == 0) {
			process_mqtt_message(conn, conns);
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
			if (shutdown(conn->pfd.fd, SHUT_RDWR) == -1) {
				err(12, "shutdown fail");
			}
			if (close(conn->pfd.fd) == -1) {
				err(1, "closing fd when deleting connection");
			}

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
			// free(conn->message);
			free(conn->client_id);
			delete_topics_list(conn->topics);
			free(conn);
		}
	}
}

void
clear_connections_signal_safe(struct connections *conns) {
	struct connection *prev, *next;
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = next
	) {
		next = conn->next;
		if (conn->delete_me) {
			if (shutdown(conn->pfd.fd, SHUT_RDWR) == -1) {
				err(12, "shutdown fail");
			}
			if (close(conn->pfd.fd) == -1) {
				err(1, "closing fd when deleting connection");
			}

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
		}
	}
}

void
sigaction_handler(int sig) {
	log_warn("Interrupt received, server terminating.");
	for (
		conn_t *conn = global_conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		conn->delete_me = 1;
	}
	clear_connections_signal_safe(global_conns);
	exit(0);
}

void
check_keep_alive(conns_t *conns) {
	for (
		conn_t *conn = global_conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (
			conn->keep_alive != 0 &&
			conn->last_seen + conn->keep_alive * 1.5 < time(NULL)
		) {
			conn->delete_me = 1;
		}
	}
}

int
main(int argc, char* argv[]) {
	int opt;
	char* portstr = "1883";

	while ((opt = getopt(argc, argv, "-p:")) != -1) {
		switch (opt) {
			case 'p':
				portstr = calloc(strlen(optarg), sizeof(char));
				if (!portstr)
					err(1, "main calloc portstr");
				portstr = strcpy(portstr, optarg);
				break;
			default:
				printf("Usage: ./mqttserver [-p <PORT>]\n");
				exit(1);
		}
		
	}

	int nclients = SIZE;

	struct sigaction sa = { 0 };
	sa.sa_handler = &sigaction_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	int sock_fd = find_connection(portstr);
	struct connections conns;
	conns_init(&conns);
	
	global_conns = &conns;

	if (listen(sock_fd, nclients) == -1)
		err(3, "listen");

	struct pollfd listening_pfd;
	listening_pfd.fd = sock_fd;
	for (;;) {
		poll_and_accept(&conns, &listening_pfd);

		check_poll_hup(&conns);
		clear_connections(&conns);
		check_poll_in(&conns);
		check_and_process_mqtt_messages(&conns);
		check_poll_out(&conns);
		check_keep_alive(&conns);
		clear_connections(&conns);
	}

	return 0;
}
