CC=cc

.PHONY:	all clean

all:
	$(CC) $(CFLAGS) $(LDFLAGS) -o client src/client.c src/crc32.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o server src/server.c src/crc32.c

clean:
	rm -rf client server *.o