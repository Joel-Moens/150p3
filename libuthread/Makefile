# Target library
lib := libuthread.a
ofiles := queue.o thread.o sem.o tps.o

CC := gcc
ARC := ar rcs
CFLAGS := -Wall -Werror -g

all: $(lib)

## TODO: Phase 1 and Phase 2
sem.o: sem.c sem.h queue.h thread.h
	$(CC) $(CFLAGS) -c -o $@ $<
tps.o: tps.c tps.h queue.h thread.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(lib): $(ofiles)
	$(ARC) $(lib) $(ofiles)

clean:
	rm -f $(lib) sem.o tps.o
