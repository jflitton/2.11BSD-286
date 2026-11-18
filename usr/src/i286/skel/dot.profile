# A new user's .profile, from /usr/skel, which adduser copies: what the
# Bourne shell runs at login.
stty dec
case "$TERM" in
pccons)	stty erase \^H;;	# the 286's screens, whose Backspace sends BS
esac
PATH=/bin:/usr/ucb:/usr/bin:/usr/local:/usr/new
export PATH
umask 22
