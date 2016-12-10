CC = gcc
CFLAGS = -g -Wall -Wno-deprecated-declarations
LDFLAGS = -lavformat -lavcodec -lavutil -lswscale -lm
INCLUDE_DIRS = -I/usr/include/ffmpeg

build: libde.so

de.o: de.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -fPIC -c $^ -o $@

libde.so: de.o
	$(CC) $(LDFLAGS) -shared $^ -o $@

clean:
	rm -rf de.o libde.so
