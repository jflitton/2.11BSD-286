; memcpy (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large (../crt/DEFS.inc).
;
; ../../compat-sys5/memcpy.c in assembly: REP MOVSW copies the words,
; and REP MOVSB the odd byte, if there is one.  n of 0 or less copies
; nothing.

%include "DEFS.inc"

T       equ     2+RA            ; the arguments, from BP: to
F       equ     T+PTRSIZE       ; from
N       equ     F+PTRSIZE       ; how many bytes

ENTRY _memcpy
        push    bp
        mov     bp, sp
        push    si
        push    di
        SAVEDS
        mov     cx, [bp+N]
        test    cx, cx
        jle     .out
        LDSSI   F
        LESDI   T
        shr     cx, 1
        rep movsw
        adc     cx, cx          ; the bit shifted out
        rep movsb
.out:   mov     ax, [bp+T]
%ifdef FARDATA
        mov     dx, [bp+T+2]
%endif
        RESTDS
        pop     di
        pop     si
        pop     bp
        RET
