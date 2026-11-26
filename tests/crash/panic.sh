# Have savecore keep crash dumps, as /etc/rc asks, and the kernel panic:
# what tests/crash has the system do first.
mkdir /usr/crash
ln -s /usr/crash /etc/crash
cc -o /tmp/crash /root/crash.s
sync
/tmp/crash
