# A new user's .cshrc, from /usr/skel, which adduser copies: what every
# C shell runs.
umask 22
set path=(/bin /usr/ucb /usr/bin /usr/local /usr/new)
if ($?prompt) then
	set history=30
	set filec
	set prompt="\!% "
endif
