#include "mqtt_connect.h"

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

char *
create_connack_message(uint8_t return_code) {
	char *buffer = calloc(4, sizeof(char));

	buffer[0] = 0x02 << 4; // control packet type
	buffer[1] = 0x02; // remaining length
	buffer[2] = 0x00; // ack flags - session present = 0
	buffer[3] = return_code;

	return buffer;
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
read_connect_message(conns_t *conns, conn_t *conn, char* incoming_message) {
	char *index = incoming_message;

    /* variable header */

	if (!check_protocol_name(index)) {
		log_error("Invalid protocol name in CONNECT variable header.");
		return 1;
	}
	index += 6;

	if (!check_protocol_level(index)) {
		log_error("Invalid protocol level in CONNECT variable header.");
		return 2;
	}
	index += 2;

	// skip flags
	index += 1;

	conn->keep_alive = get_keep_alive(index);

    /* payload */

	char *client_id = get_client_id(index);

	if (strlen(client_id) == 0) {
		log_warn("Found empty client ID.");
		return 3;
	}

	if (check_for_client_id_repeated(conns, client_id)) {
		log_warn("Found duplicate client ID.");
		return 3;
	}

	log_debug("client id: %s", client_id);

	conn->client_id = client_id;

	return 0;
}

char *
create_connect_response(conn_t *conn, conns_t *conns, int code) {
	if (code == 0) {
		// CONNACK OK
		log_info("CONNACK OK");
		conn->message_size = 4;
		return create_connack_message(0x00);
	}
	else if (code == 2) {
		// CONNACK invalid protocol version (only v3.1.1 is supported)
		log_info("CONNACK invalid protocol version");
		conn->message_size = 4;
		conn->delete_me = 1;
		return create_connack_message(0x01);
	}
	else if (code == 3) {
		// CONNACK invalid identifier
		log_info("CONNACK invalid identifier");
		conn->message_size = 4;
		conn->delete_me = 1;
		return create_connack_message(0x02);
	}
	else {
		// error, just disconnect
		log_warn("error, disconnect");
		conn->delete_me = 1;
	}

	return NULL;
}
