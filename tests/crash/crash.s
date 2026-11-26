; Make system call 51, which no program makes: tests/crash has given it
; a function in no segment, and the kernel faults calling it.
        bits    16
        cpu     286
        segment _TEXT public align=2 class=CODE
        global  _main
_main:  mov     ax, 51
        int     0x30
        ret
