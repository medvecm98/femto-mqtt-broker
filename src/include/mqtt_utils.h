#ifndef FEMTO_MQTT_UTILS_H
#define FEMTO_MQTT_UTILS_H

#include "structs.h"

int
check_for_client_id_repeated(struct connections *conns, char* client_id);

int
from_val_len_to_uint(char *buffer);

char *
from_uint_to_val_len(int val);

#endif
