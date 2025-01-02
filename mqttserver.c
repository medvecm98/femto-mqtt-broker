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

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

struct connection {
	struct pollfd pfd;
	struct connection *next;
	struct connection *prev;
	char *topic;
	// int connection_accepted;
	// int next_topic_index;
};

struct connections {
	struct connection *conn_head;
	struct connection *conn_back;
	int count;
};

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
		conns->conn_head->next = new_connection;
	}
	else {
		conns->conn_head = new_connection;
	}
	new_connection->prev = conns->conn_head;

	// set new head
	conns->conn_head = new_connection;

	conns->count++;

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

void
process_write_from_client(struct connection *conn) {
	if (write(conn->pfd.fd, "hello\n", 7) == -1)
		err(1, "write");
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
		struct connection* conn = conns->conn_head;
		conn != NULL;
		conn = conn->next
	) {
		printf("    [%d] fd: %d\n", i++, conn->pfd.fd);
	}
}

void
check_poll_in(struct connections *conns) {
	if (conns->count == 0) return;
	printf("checking POLLIN\n");

	struct pollfd *pfds = calloc(conns->count, sizeof(struct pollfd));
	if (!pfds)
		err(1, "pfds calloc");
	
	int i = 0;
	for (
		struct connection* conn = conns->conn_head;
		conn != NULL;
		conn = conn->next
	) {
		pfds[i] = conn->pfd;
		i++;
	}

	if (poll(pfds, conns->count, 1000) == -1)
		err(1, "poll");

	for (
		struct connection* conn = conns->conn_head;
		conn != NULL;
		conn = conn->next
	) {
		if (conn->pfd.revents & POLLIN) {
			printf("Found POLLIN\n");
			process_write_from_client(conn);
		}
	}

	free(pfds);
}

void
poll_and_accept(struct connections *conns, struct pollfd *listening_pfd) {
	int nfd = -1;
	listening_pfd->events = POLLIN;

	if (poll(listening_pfd, 1, 1000) == -1)
		err(1, "poll listening pfd");

	if (listening_pfd->revents & POLLIN) {
		if ((nfd = accept(listening_pfd->fd, NULL, NULL)) == -1)
			err(3, "accept");
		printf("accepted\n");
		add_connection(conns, nfd);
		printf("connection added\n");
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

		check_poll_in(&conns);
		// check_poll_hup(conns);
	}

	return 0;
}
