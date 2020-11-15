CC=gcc
CFLAGS=
DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: sender receiver


sender: sender.o
	$(CC) -o sender sender.o

receiver.o: receiver.c
	$(CC) $(CFLAGS) -c receiver.c