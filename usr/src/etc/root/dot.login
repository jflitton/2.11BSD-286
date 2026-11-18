unset noglob
set path=(/bin /etc /sbin /usr/sbin /usr/ucb /usr/bin /usr/local /usr/new)
stty dec kill ^U eof ^D
if ("$TERM" == pccons) stty erase ^H	# the 286's screens send BS
set prompt="[\!] root--> "

if ( "$TERM" == dialup || "$TERM" == network) then
  setenv TERM vt100
  echo -n "Terminal ($TERM)? "
  set argv = "$<"
  if ( "$argv" != '' ) setenv TERM "$argv"
endif

biff y
