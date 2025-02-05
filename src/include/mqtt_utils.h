#ifndef FEMTO_MQTT_UTILS_H
#define FEMTO_MQTT_UTILS_H

#include "structs.h"

/**
 * Checks if client IDs were already used by other connected clients.
 * 
 * NOTE: Run this BEFORE inserting newly connected client into connections 
 * linked list, otherwise it will match itself.
 */
int
check_for_client_id_repeated(struct connections *conns, char* client_id);

/**
 * Convert variable length integer to C integer.
 * 
 * Use in remaining length field in MQTT control packet fixed header.
 */
int
from_val_len_to_uint(char *buffer);

/**
 * Convert C integer to variable length integer.
 * 
 * Use in remaining length field in MQTT control packet fixed header.
 */
char *
from_uint_to_val_len(int val);

#endif
