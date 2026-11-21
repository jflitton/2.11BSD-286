; modf(x, &ip): the fraction of x, with its whole part stored at ip, on
; the 287 (NASM: small model, with -DFAR medium, or with -DFARDATA too
; large).  Both have the sign of x.

%include "DEFS.inc"

RC_CHOP equ     0x0C00          ; rounding control: toward zero

        global  _modf

_modf:  push    bp
        mov     bp, sp
        sub     sp, 4
        fstcw   [bp-2]          ; the control word as it is
        fwait
        mov     ax, [bp-2]
        or      ax, RC_CHOP
        mov     [bp-4], ax
        fld     qword [bp+2+RA] ; x
        fldcw   [bp-4]
        fld     st0
        frndint                 ; whole, x
        fldcw   [bp-2]
%ifdef FARDATA
        lds     bx, [bp+2+RA+8] ; ip, far
%else
        mov     bx, [bp+2+RA+8] ; ip, near
%endif
        fst     qword [bx]
        fxch                    ; x, whole
        fsub    st0, st1        ; x - whole, whole
        fstp    st1
        fwait                   ; the store, before the caller looks
        mov     sp, bp
        pop     bp
        RET
