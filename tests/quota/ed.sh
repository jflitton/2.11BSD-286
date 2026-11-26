#!/bin/sh
# The editor tests/quota has edquota run on its file, $1: keep what it
# was given, and give quser limits on /home of 20K and 30K, and of 5
# and 8 files, in place of none.  The file is written over, not made
# again: edquota reads it back through what it has open.
cp $1 /tmp/ed.out
sed -e '2s/soft = 0, hard = 0/soft = 20, hard = 30/' \
    -e '3s/soft = 0, hard = 0/soft = 5, hard = 8/' /tmp/ed.out > $1
