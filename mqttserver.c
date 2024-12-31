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

#define SIZE 256

struct connection {
	struct pollfd pfd;
	struct connection *next;
	struct connection *prev;
	char *topic;
	// int next_topic_index;
};

struct connections {
	struct connection *conn_head;
	struct connection *conn_back;
	int count;
};

void
add_connection(struct connections conns, int fd) {
	struct connection *new_connection = calloc(1, sizeof(struct connection));
	new_connection->next = NULL;
	new_connection->prev = NULL;
	// new_connection->next_topic_index = 0;
	// memset(&new_connection->pfd, 0, sizeof(struct pollfd));

	// set 'back' if it is NULL
	if (!conns.conn_back) {
		conns.conn_back = new_connection;
	}

	// set prev/next
	conns.conn_head->next = new_connection;
	new_connection->prev = conns.conn_head;

	// set new head
	conns.conn_head = new_connection;

	conns.count++;

	new_connection->pfd.fd = fd;
	new_connection->pfd.events = POLLIN;
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

	for (info = info_orig; info != NULL; info = info->ai_next) {
		sock_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		if (!bind(sock_fd, info->ai_addr, info->ai_addrlen)) break;
	}

	freeaddrinfo(info_orig);

	return sock_fd;
}

void
conns_init(struct connections conns) {
	memset(&conns, 0, sizeof(conns));
	conns.count = 0;
	conns.conn_back = NULL;
	conns.conn_head = NULL;
}

void
process_write_from_client(struct connection *conn) {
	if (write(conn->pfd.fd, "hello\n", 7))
		err(1, "write");
}

void
check_poll_in(struct connections conns) {
	struct pollfd *pfds = calloc(conns.count, sizeof(struct pollfd*));
	
	int i = 0;
	for (
		struct connection* conn = conns.conn_head;
		conn != NULL;
		conn = conn->next
	) {
		pfds[i] = conn->pfd;
		i++;
	}

	if (poll(pfds, 1, 1000) == -1)
		err(1, "poll");

	for (
		struct connection* conn = conns.conn_head;
		conn != NULL;
		conn = conn->next
	) {
		if (conn->pfd.revents | POLLIN) {
			process_write_from_client(conn);
		}
	}

	free(pfds);
}

int
main(int argc, char* argv[]) {
	char* portstr = "1883";
	int nclients = SIZE;

	int sock_fd = find_connection(portstr);
	struct connections conns;
	conns_init(conns);

	if (listen(sock_fd, nclients) == -1)
		err(3, "listen");

	int nfd = 0;
	for (;;) {
		if ((nfd = accept(sock_fd, NULL, NULL)) == -1)
			err(3, "accept");
		add_connection(conns, nfd);
	}

	return 0;
}
