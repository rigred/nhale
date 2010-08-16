CC = gcc
CFLAGS = -Werror -Wall -Wextra -Wno-unused-parameter
AR = ar
OBJECTS = back_linux.o bios.o info.o crc32.o
DEPS = libbackend.a

.PHONY: clean

nhale:  $(DEPS) nhale.c
	$(CC) $(CFLAGS) nhale.c $(DEPS) -o nhale

libbackend.a: $(OBJECTS)
	$(AR) crus libbackend.a $(OBJECTS)

back_linux.o: back_linux.c back_linux.h info.h backend.h
	$(CC) -c $(CFLAGS) back_linux.c

bios.o: bios.c bios.h info.h crc32.h backend.h
	$(CC) -c $(CFLAGS) bios.c

info.o: info.c info.h backend.h
	$(CC) -c $(CFLAGS) info.c

crc32.o: crc32.c crc32.h
	$(CC) -c $(CFLAGS) crc32.c

clean :
	rm -f *.o *.a *~ nhale