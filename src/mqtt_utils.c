#include "mqtt_utils.h"

int
check_for_client_id_repeated(struct connections *conns, char* client_id) {
	for (
		struct connection* conn = conns->conn_back;
		conn != NULL;
		conn = conn->next
	) {
		if (conn->client_id &&
			strncmp(client_id, conn->client_id, conn->cliend_id_length) == 0 &&
			conn->delete_me != 1
		) {
			/** we only want to remove clients with matching client_id 
			 * that are not already being removed
			 */
			return 1;
		}
	}

	return 0;
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
	} while ((encoded_byte & 128) != 0);

	return value;
}

char *
from_uint_to_val_len(int val, size_t *len) {
	char encoded_byte;
	char *output = calloc(5, sizeof(char));
	if (!output)
		err(1, "uint to val len calloc output");
	int index = 0;

	do {
		encoded_byte = val % 128;
		val = val / 128;
		// if there are more data to encode, set the top bit of this byte
		if (val > 0)
			encoded_byte = encoded_byte | 128;
		output[index++] = encoded_byte;
	} while (val > 0);

	*len = index;
	return output;
}
