all: mqttserver

mqttserver: mqttserver.c
	$(CC) -Wall -std=c99 -Werror=pedantic -D_POSIX_C_SOURCE=200112L -O0 -g mqttserver.c -o mqttserver
