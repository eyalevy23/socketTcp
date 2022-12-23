# Makefile for TCP project

all: tcp-server tcp-client

tcp-server: Receiver.c
	gcc -o tcp-server Receiver.c

tcp-client: Sender.c
	gcc -o tcp-client Sender.c

clean:
	rm -f *.o tcp-server tcp-client

runs:
	./tcp-server

runc:
	./tcp-client

runs-strace:
	strace -f ./tcp-server

runc-strace:
	strace -f ./tcp-client

new: clean all