CC = gcc

CFLAGS = -Wall -pedantic -g

LD = gcc

LDFLAGS = 

.SILENT:

all: mush2

mush2: mush2.o
	$(LD) $(LDFLAGS) -L ~pn-cs357/Given/Mush/lib64 mush2.o -o mush2 -lmush

mush2.o: mush2.c
	$(CC) $(CFLAGS) -I ~pn-cs357/Given/Mush/include -c mush2.c -o mush2.o

test:
	valgrind --leak-check=yes ./mush2

clean:
	rm -f *.o mush2
