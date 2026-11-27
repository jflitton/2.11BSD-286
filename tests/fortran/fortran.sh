# What tests/fortran has the system do (tests/fortran/run.sh): f77 on
# the programs beside this, which are then run; ratfor's and struct's
# through f77, and efl's tests; and what f77 makes of libU77's ioinit.f
# against tools/build/ioinit.s.  What f77 says of a file is said only if
# it fails.
cd /root
echo "== control"
f77 -o t1 t1.f > t1.log 2>&1 || cat t1.log
./t1 > t1.out
cat t1.out
f77 -O1 -o t1o t1.f > t1o.log 2>&1 || cat t1o.log
./t1o > t1o.out
cmp -s t1.out t1o.out && echo "the same with -O1, a register variable"
echo "== arithmetic"
f77 -o t2 t2.f > t2.log 2>&1 || cat t2.log
./t2
echo "== subprograms"
f77 -c t3s.f > t3s.log 2>&1 || cat t3s.log
f77 -O -o t3 t3.f t3s.o > t3.log 2>&1 || cat t3.log
./t3
echo "== characters and files"
f77 -o t4 t4.f > t4.log 2>&1 || cat t4.log
./t4 <<EOF
21 1.25
SHELL
EOF
echo "== ratfor"
f77 -o t5 t5.r > t5.log 2>&1 || cat t5.log
./t5
echo "== struct"
f77 -o t6 t6.f > t6.log 2>&1 || cat t6.log
./t6
struct t6.f > t6s.r
f77 -o t6s t6s.r > t6s.log 2>&1 || cat t6s.log
./t6s
echo "== efl"
cd efltest
for t in george select matmul
do
	efl "system=unix" $t.e > $t.f 2> $t.err
	cmp -s $t.f $t.out && echo "efl $t.e as $t.out has it"
done
f77 -c george.f > george.log 2>&1 && echo "f77 -c george.f" || cat george.log
cd /root
echo "== ioinit"
f77 -O -S ioinit.f > ioinit.log 2>&1 || cat ioinit.log
if cmp -s ioinit.s ioinit.ref
then
	echo "ioinit.s as tools/build/ioinit.s has it"
else
	echo "== ioinit.s"
	uuencode ioinit.s ioinit.s
	echo "== ioinit.s end"
fi
echo "== end"
