# What tests/quota has quser do in its directory.  With "fill": write
# past the soft limit of 20K, and on to the hard one of 30K, where a
# write is refused; and make files past the soft limit of 5, and on to
# the hard one of 8, where making one is refused.  With "over", once
# three logins have used up the warnings: the soft limits are hard now,
# so that nothing more may be written or made, until files are removed.
# The kernel says so once for each until something is freed, which c is
# first, and when b is, says how far under the soft limit quser is.
cd /home/quser
case "$1" in
fill)
	echo "== fill"
	dd if=/unix of=a bs=1k count=12 2>&1
	dd if=/unix of=b bs=1k count=10 2>&1
	dd if=/unix of=c bs=1k count=10 2>&1
	touch d e f g 2>&1
	quota
	;;
over)
	echo "== over"
	rm c
	dd if=/unix bs=1k count=1 conv=notrunc 2>&1 >>a
	touch h 2>&1
	rm b
	quota
	;;
esac
