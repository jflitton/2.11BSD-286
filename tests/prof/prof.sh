# What tests/prof has the system do (tests/prof/run.sh): cc -p on t.c,
# its run, which writes mon.out, and what prof makes of it; the same
# objects linked without -p, which write none; cc -p of the medium
# model, which it refuses; and f77 -p.  Of prof's table only the calls
# counted of some functions are said, which are the same each time, and
# whether fib, which takes the most of the program's time, had any;
# the times are not.  A function prof has counted the calls of is a
# line of five fields in its table; what prof says besides, that f,
# which ends at once, had no time when no tick fell in it, is not.  The
# shell has no functions.  After the end, prof's table of t, to be read
# and not compared.
cd /root
echo "== cc -p"
cc -p -O -o t t.c > t.log 2>&1 || cat t.log
rm -f mon.out
./t
test -f mon.out && echo "mon.out"
cp mon.out t.mon
prof t > table 2>&1
for n in _main _fib _sq _abs _getpid _half
do
	awk 'NF == 5 && $5 == "'$n'" { print $5, $3; found = 1 }
	    END { if (!found) print "'$n' none" }' < table
done
awk '$5 == "_fib" && $1 > 0 { print "_fib had time" }' < table
prof -a t > table 2>&1
awk 'NF == 5 && $5 == "_half" { print $5, $3; found = 1 }
    END { if (!found) print "_half none" }' < table
echo "== not linked with -p"
cc -p -O -c t.c
cc -o t2 t.o
rm -f mon.out
./t2
test -f mon.out || echo "no mon.out"
echo "== medium"
cc -p -mm -o t3 t.c
echo "exit $?"
echo "== f77 -p"
f77 -p -o f f.f > f.log 2>&1 || cat f.log
rm -f mon.out
./f
prof f > table 2>&1
for n in _MAIN_ _bump_
do
	awk 'NF == 5 && $5 == "'$n'" { print $5, $3; found = 1 }
	    END { if (!found) print "'$n' none" }' < table
done
echo "== end"
prof t t.mon
