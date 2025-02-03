all: mqttserver

obj/log.o: src/include/log.h src/log.c
	$(CC) -c -I src/include -o obj/log.o src/log.c

obj/mqtt_connect.o: src/include/mqtt_connect.h src/mqtt_connect.c
	$(CC) -c -I src/include -o obj/mqtt_connect.o src/mqtt_connect.c

obj/mqtt_publish.o: src/include/mqtt_publish.h src/mqtt_publish.c
	$(CC) -c -I src/include -o obj/mqtt_publish.o src/mqtt_publish.c

obj/mqtt_subscribe.o: src/include/mqtt_subscribe.h src/mqtt_subscribe.c
	$(CC) -c -I src/include -o obj/mqtt_subscribe.o src/mqtt_subscribe.c

obj/mqtt_utils.o: src/include/mqtt_utils.h src/mqtt_utils.c
	$(CC) -c -I src/include -o obj/mqtt_utils.o src/mqtt_utils.c

obj/topic_list.o: src/include/topic_list.h src/topic_list.c
	$(CC) -c -I src/include -o obj/topic_list.o src/topic_list.c

mqttserver: obj/log.o obj/mqtt_connect.o obj/mqtt_publish.o obj/mqtt_subscribe.o obj/mqtt_utils.o obj/topic_list.o src/mqttserver.c
	$(CC) -Wall -std=c99 -Werror=pedantic -D_POSIX_C_SOURCE=200809L -I src/include -O0 -g src/mqttserver.c obj/log.o obj/mqtt_connect.o obj/mqtt_publish.o obj/mqtt_subscribe.o obj/mqtt_utils.o obj/topic_list.o -o mqttserver

clean:
	rm mqttserver
