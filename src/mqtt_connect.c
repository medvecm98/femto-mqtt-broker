#include "mqtt_connect.h"

/**
 * Parses 16-bit BE into int keep alive value.
 */
int
get_keep_alive(char *index) {
	int keep_alive = *index;
	keep_alive <<= 8;
	keep_alive = keep_alive | *(index + 1);

	return keep_alive;
}

/**
 * Reads length (from 16-bit BE) of client ID, allocates buffer and reads it.
 * 
 * \returns Client ID in allocated buffer.
 */
char*
get_client_id(char *index) {
	uint8_t *index8 = (uint8_t*) index;
	uint16_t client_id_length = *index8;
	index8++;
	client_id_length <<= 8;
	client_id_length |= *index8;
	index8++;
	
	char *client_id = calloc(client_id_length + 1, 1);
	if (!client_id)
		err(1, "get client_id calloc client_id");
	memcpy(client_id, index8, client_id_length);

	return client_id;
}

/**
 * \returns Allocated buffer with CONNACK message in bytes form.
 */
char *
create_connack_message(uint8_t return_code) {
	char *buffer = calloc(4, sizeof(char));
	if (!buffer)
		err(1, "create connack calloc buffer");

	buffer[0] = 0x02 << 4; // control packet type
	buffer[1] = 0x02; // remaining length
	buffer[2] = 0x00; // ack flags - session present = 0
	buffer[3] = return_code;

	return buffer;
}

/**
 * Checks protocol name in CONNECT variable header.
 * 
 * \returns 1 if it checks out, 0 otherwise.
 */
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

/**
 * Checks supported protocol level in CONNECT variable header.
 * 
 * \returns 1 if it checks out, 0 otherwise.
 */
int
check_protocol_level(char *index) {
	if (*index == 4) {
		return 1;
	}
	else
		return 0;
}

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
	index += 1;

	// skip flags
	index += 1;

	conn->keep_alive = get_keep_alive(index);
	index += 2;

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

	conn->client_id = client_id;

	return 0;
}

/**
 * Creates CONNACK MQTT control packet based on provided code.
 * 
 * \returns CONNACK MQTT control packet in bytes form.
 */
char *
create_connect_response(conn_t *conn, conns_t *conns, int code) {
	if (code == 0) {
		// CONNACK OK
		conn->message_size = 4;
		return create_connack_message(0x00);
	}
	else if (code == 2) {
		// CONNACK invalid protocol version (only v3.1.1 is supported)
		conn->message_size = 4;
		conn->delete_me = 1;
		return create_connack_message(0x01);
	}
	else if (code == 3) {
		// CONNACK invalid identifier
		conn->message_size = 4;
		conn->delete_me = 1;
		return create_connack_message(0x02);
	}
	else {
		// error, just disconnect
		log_warn("CONNECT error, disconnect");
		conn->delete_me = 1;
	}

	return NULL;
}
