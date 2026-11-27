# What tests/pascal has the system do (tests/pascal/run.sh): pi on the
# programs beside this, and what it makes of each run, by px, as an
# object runs itself and by pix; the errors px finds, one at a time; and
# pxp printing a program again, and with its profile.  What pi says of a
# program is said only if it fails; how many seconds px says a program
# ran for, and the dates of pxp's heading, are not said, nor how deep
# a program that recurses for ever went before px's stack ran out.  The
# input of errors is a file: sh says a newline for a process a signal
# ended, and echo's to a pipe px has not read can end with SIGPIPE.
cd /root
secs='s/ in [0-9]*\.[0-9][0-9] seconds/ in N seconds/'
echo "== control"
pi t1.p > t1.log 2>&1 || cat t1.log
mv obj t1
./t1 2>&1 | sed "$secs"
echo "== arithmetic"
pi t2.p > t2.log 2>&1 || cat t2.log
mv obj t2
px t2 2>&1 | sed "$secs"
echo "== structures"
pix t3.p 2>&1 | sed "$secs"
echo "== files"
pi t4.p > t4.log 2>&1 || cat t4.log
mv obj t4
./t4 2>&1 <<EOF | sed "$secs"
21 1.25
-7 and the rest
echo this line
EOF
for f in numbers items data tmp.1 tmp.2
do
	test -f $f && echo "left $f"
done
echo "== builtins"
pi t5.p > t5.log 2>&1 || cat t5.log
mv obj t5
px t5 one two 2>&1 | sed "$secs"
echo "== errors"
pi errors.p > errors.log 2>&1 || cat errors.log
mv obj errors
echo abc > abc
for e in range div mod real overflow nil case subrange chr sqrt ln read \
    eof halt stlimit argv none
do
	echo "-- $e"
	px errors $e < abc 2>&1 | sed "$secs"
done
echo "-- stack"
px errors stack 2>&1 | uniq | sed -e "$secs" -e 's/^[0-9]* statements/N statements/'
echo "== pxp"
pxp prof.p
pi -z prof.p > prof.log 2>&1 || cat prof.log
px obj > /dev/null 2>&1
pxp -z prof.p | sed '1,/^Profiled /d'
echo "== end"
