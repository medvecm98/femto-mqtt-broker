all: mqttserver

mqttserver: mqttserver.c log.c log.h
	$(CC) -Wall -std=c99 -Werror=pedantic -D_POSIX_C_SOURCE=200112L -O0 -g mqttserver.c log.c -o mqttserver
