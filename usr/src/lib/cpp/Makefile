#	Makefile	2.3	(2.11BSD) 2025/11/25
#

# The 80286's keeps what it has in memory, LIBVMF being 0, and not in a
# file of libvmf's: the text of its macros and their names in segments
# it asks the system for (segalloc(2)), which tcsh's headers, and the
# kernel's, need, and the rest in its own data.
LDFLAGS=-i

CFLAGS=-O -DLIBVMF=0 -I.

all : cpp cpp.0

cpp :	cpp.o cpc.o token.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o cpp cpp.o cpc.o token.o

cpp.0 : cpp.1
	/usr/man/manroff cpp.1 > cpp.0

clean :
	rm -f *.o cpp cpp.0

install : cpp cpp.0
	install -s -m 755 -o root -g wheel cpp /lib
	install -m 444 -o root -g staff cpp.0 /usr/man/cat1
