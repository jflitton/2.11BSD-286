# A new user's .login, from /usr/skel, which adduser copies: what the
# C shell runs at login, after .cshrc.
stty dec
if ("$TERM" == pccons) stty erase ^H	# the 286's screens send BS
