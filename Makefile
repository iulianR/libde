CC = gcc
CFLAGS = -g -Wall -Wno-deprecated-declarations
# LDFLAGS = -Llibraries -lm -lavcodec -lavutil
LDFLAGS = -lavformat -lavcodec -lavutil -lswscale -lm
INCLUDE_DIRS = -I/usr/include/ffmpeg

build: main

de.o: de.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -fPIC -c $^ -o $@

libde.so: de.o
	$(CC) $(LDFLAGS) -shared $^ -o $@

main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c main.c

main: main.o libde.so
	$(CC) -L. -lde $(LDFLAGS) main.o -o main

clean:
	rm -rf de.o libde.so main.o main *.ppm