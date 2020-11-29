CC=gcc
CFLAGS=
DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: sender receiver
	

sender: sender.o
	$(CC) -c -fpic sharedmem.c
	$(CC) -shared -o libsharedmem.so sharedmem.o -lrt
	$(CC) -L. -o sender sender.o -lsharedmem -lrt

receiver: receiver.o
	
	$(CC) -L. -o receiver receiver.o -lsharedmem -lrt

runsender:
	@LD_LIBRARY_PATH=./ ./sender

runreceiver:
	@LD_LIBRARY_PATH=./ ./receiver