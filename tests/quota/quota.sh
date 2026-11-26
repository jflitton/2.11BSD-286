# What tests/quota has root do.  /etc/rc has already made /home's quota
# file with quotacheck and turned its quotas on with quotaon, fstab
# saying rq for it.  With no argument: give quser limits on /home, and
# root the same with edquota -p, which keeps root's own counts, and
# have user.sh fill quser's as quser.  With "check": turn the quotas off,
# give quser a file the kernel does not count, have quotacheck count it
# and turn them on again.  With "after", once the system has started
# again: what is left.
case "$1" in
"")
	echo "== quota"
	mount | grep /home
	ls /home
	repquota /home
	echo "== edquota"
	EDITOR=/root/ed.sh edquota quser
	cat /tmp/ed.out
	quota -v quser
	edquota -p quser root
	repquota /home
	su quser /root/user.sh fill
	;;
check)
	echo "== check"
	quotaoff -v /home
	cp /etc/motd /home/quser/m
	chown quser /home/quser/m
	quotacheck -v /home
	quotaon -v /home
	repquota /home
	echo "== rebooting"
	;;
after)
	echo "== after"
	mount | grep /home
	repquota /home
	quota -v quser
	echo "== end"
	;;
esac
