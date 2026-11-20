#!/bin/sh
# 10-Oct-2026 - tmk - initial version
#
# The following massages the disktab file and removes
# all information except the drive model(s) and par-
# tition tables. It also collapses multiple blank lines
# and removes the trailing ram disk entry.
#
# Suggestions and/or patches are welcome.
#
# Long annoying command string to tease sms who said
# I should write a program and use getfsent() instead.
#
grep -v ^# disktab | sed 's/:\\$//' | sed -E 's/:(b|f)[^#]*#1024//g' | \
grep -v :b0 | grep -v :dt | grep -v :ty | grep -v "Ram disk" | \
sed -E '
  s/:$//;
  s/:p(.)#([^:]*):o.#([^:]*):t.=(.*)/Partition \1: size=\2, offset=\3, type=\4/;
  s/:p(.)#([^:]*):o.#([^:]*)/Partition \1: size=\2, offset=\3/;
  s/:p(.)#([^:]*)/Partition \1: size=\2/' | \
  sed $'s/^        /\t/' | cat -s - > disktab.out
