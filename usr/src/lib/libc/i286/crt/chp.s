; __CHP: round the top of the 287 stack to an integer, toward zero.
;
; The compiler calls this before FISTP when it converts a float or double
; to an integer: C truncates, and the 287 normally rounds to nearest.  All
; registers are preserved, and the control word is left as it was found.

%include "DEFS.inc"

ENTRY __CHP
        push    bp
        mov     bp, sp
        sub     sp, 4
        push    ax
        fstcw   [bp-2]          ; the caller's control word
        fwait
        mov     ax, [bp-2]
        or      ah, 0x0C        ; rounding control = chop
        mov     [bp-4], ax
        fldcw   [bp-4]
        frndint
        fldcw   [bp-2]
        pop     ax
        mov     sp, bp
        pop     bp
        RET
