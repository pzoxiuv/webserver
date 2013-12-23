CC=gcc
CFLAGS=-Wall -I/usr/include/lua5.1/
LDFLAGS=-llua5.1
OUT=server

all: server

server:
	$(CC) $(CFLAGS) -o $(OUT) server.c $(LDFLAGS)

clean:
	rm server
