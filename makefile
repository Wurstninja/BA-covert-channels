CC=gcc
CFLAGS=
DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

testmake: test.o
	$(CC) -o testmake test.o