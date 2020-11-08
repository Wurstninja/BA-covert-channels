CC=gcc
CFLAGS=
DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: test sender receiver


test: test.o
	$(CC) -o test test.o

sender: sender.o
	$(CC) -o sender sender.o

receiver.o: receiver.c
	$(CC) $(CFLAGS) -c receiver.c