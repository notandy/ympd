CFLAGS = -Wall -Os
LFLAGS = `pkg-config --libs libwebsockets libmpdclient`

.PHONY: clean

ympd: main.o http_server.o mpd_client.o
	$(CC) $(LFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f main.o http_server.o mpd_client.o ympd
