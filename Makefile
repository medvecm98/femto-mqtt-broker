all: mqttserver

HEADERS = $(wildcard src/include/*.h)
SOURCES = $(wildcard src/*.c)

mqttserver: $(SOURCES) $(HEADERS)
	$(CC) -Wall -std=c99 -Werror=pedantic -D_POSIX_C_SOURCE=200809L -I src/include -O0 -g $(SOURCES) -o mqttserver

clean:
	rm mqttserver
