all: mqttserver

mqttserver: mqttserver.c
	$(CC) -Wall -std=c99 -Werror=pedantic mqttserver.c -o mqttserver
