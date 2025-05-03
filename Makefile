all: mqttserver

CFLAGS = -Wall -std=c99 -Werror=pedantic -D_POSIX_C_SOURCE=200809L -I src/include -O0 -g

src/log.o: src/include/log.h src/log.c
	pwd
	$(CC) -c $(CFLAGS) -o src/log.o src/log.c

src/poll_list.o: src/include/poll_list.h src/poll_list.c
	$(CC) -c $(CFLAGS) -o src/poll_list.o src/poll_list.c

src/mqtt_connect.o: src/include/mqtt_connect.h src/mqtt_connect.c
	$(CC) -c $(CFLAGS) -o src/mqtt_connect.o src/mqtt_connect.c

src/mqtt_publish.o: src/include/mqtt_publish.h src/mqtt_publish.c
	$(CC) -c $(CFLAGS) -o src/mqtt_publish.o src/mqtt_publish.c

src/mqtt_subscribe.o: src/include/mqtt_subscribe.h src/mqtt_subscribe.c
	$(CC) -c $(CFLAGS) -o src/mqtt_subscribe.o src/mqtt_subscribe.c

src/mqtt_utils.o: src/include/mqtt_utils.h src/mqtt_utils.c
	$(CC) -c $(CFLAGS) -o src/mqtt_utils.o src/mqtt_utils.c

src/topic_list.o: src/include/topic_list.h src/topic_list.c
	$(CC) -c $(CFLAGS) -o src/topic_list.o src/topic_list.c

mqttserver: src/log.o src/mqtt_connect.o src/mqtt_publish.o src/mqtt_subscribe.o src/mqtt_utils.o src/topic_list.o src/mqttserver.c src/poll_list.o
	$(CC) $(CFLAGS) src/mqttserver.c src/log.o src/mqtt_connect.o src/mqtt_publish.o src/mqtt_subscribe.o src/mqtt_utils.o src/topic_list.o -o mqttserver

clean:
	rm -f mqttserver
	rm -f src/*.o
