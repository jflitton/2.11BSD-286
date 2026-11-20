#
# adventure, made on the system, as upstream's Makefile makes it
# (usr/src/games/adventure/Makefile), but for compiling a.out, which is
# built with the rest of the system.  Run once, a.out reads the game's
# text, glorkz, saves itself with what it has read as advent, and puts
# the messages after it; advent is then /usr/games/adventure.  As root:
#
#	cd /usr/games/lib/adventure; make install
#
advent:	a.out glorkz
	rm -f advent
	echo y | ./a.out

install: advent
	install -c -s -m 755 advent /usr/games/adventure

clean:
	rm -f advent
