stty dec
case "$TERM" in
pccons)	# the 286's screens, whose Backspace sends BS
	echo 'erase ^H, kill ^U, intr ^C'
	stty erase \^H;;
*)	echo 'erase DEL, kill ^U, intr ^C';;
esac
PATH=/bin:/sbin:/usr/sbin:/etc:/usr/ucb:/usr/bin:/usr/new
export PATH
HOME=/root
export HOME
export TERM
umask 22
