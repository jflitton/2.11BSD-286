; The process's executable, as data in the kernel half of tests/user.
; There is no file system to read it from yet.

%include "mch_defs.inc"

        global  _userprog
        DATA
_userprog:
        incbin  "user"
