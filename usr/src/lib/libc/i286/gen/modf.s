; modf(x, &ip): the fraction of x, with its whole part stored at ip, on
; the 287 (NASM, small model).  Both have the sign of x.

        bits    16
        cpu     286

RC_CHOP equ     0x0C00          ; rounding control: toward zero

        segment _TEXT public align=2 class=CODE
        global  _modf

_modf:  push    bp
        mov     bp, sp
        sub     sp, 4
        fstcw   [bp-2]          ; the control word as it is
        fwait
        mov     ax, [bp-2]
        or      ax, RC_CHOP
        mov     [bp-4], ax
        fld     qword [bp+4]    ; x
        fldcw   [bp-4]
        fld     st0
        frndint                 ; whole, x
        fldcw   [bp-2]
        mov     bx, [bp+12]
        fst     qword [bx]
        fxch                    ; x, whole
        fsub    st0, st1        ; x - whole, whole
        fstp    st1
        fwait                   ; the store, before the caller looks
        mov     sp, bp
        pop     bp
        ret
