; ldexp(x, n): x times 2 to the n, on the 287 (NASM: small model, or
; with -DFAR medium).  The result is on the 287's stack, as compiled code
; expects a double.

%include "DEFS.inc"

        global  _ldexp

_ldexp: push    bp
        mov     bp, sp
        fild    word [bp+2+RA+8] ; n
        fld     qword [bp+2+RA] ; x
        fscale
        fstp    st1
        pop     bp
        RET
