#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)newvers.sh	1.7 (2.11BSD) 2025/12/26
#
# The 286's: the kernel says what it is and the day it was made, or what
# DATE is in the environment; the same sources made on the same day, or
# with the same DATE, are the same kernel wherever they are made.
#
if [ ! "$DATE" ]; then
	DATE=`date | awk '{ m = index("JanFebMarAprMayJunJulAugSepOctNovDec", $2)
	    printf "%s-%02d-%02d", $6, (m + 2) / 3, $3 }'`
fi
echo "char version[] = \"2.11 BSD UNIX for the 286 ($DATE)\";" > vers.c
