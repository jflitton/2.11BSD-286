; ldexp(x, n): x times 2 to the n, on the 287 (NASM, small model).  The
; result is on the 287's stack, as compiled code expects a double.

        bits    16
        cpu     286

        segment _TEXT public align=2 class=CODE
        global  _ldexp

_ldexp: push    bp
        mov     bp, sp
        fild    word [bp+12]    ; n
        fld     qword [bp+4]    ; x
        fscale
        fstp    st1
        pop     bp
        ret
