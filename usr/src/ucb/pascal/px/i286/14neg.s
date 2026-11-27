;
; NEGATION & ABSOLUTE VALUE
;
; Of an integer of 2 bytes the result is a long, so that of -32768 is
; 32768, where the PDP-11's was -32768.
;
_ABS2:
        pop     ax
        cwd
        test    dx, dx
        jns     abs21
        jmp     neg21
_NEG2:
        pop     ax
        cwd
neg21:
        neg     dx
        neg     ax
        sbb     dx, 0
abs21:
        push    dx
        push    ax
        NEXT
_ABS4:
        mov     bx, sp
        cmp     word [bx+2], 0
        jge     .1
        jmp     _NEG4
.1:
        NEXT
_NEG4:
        pop     ax
        pop     dx
        neg     dx
        neg     ax
        sbb     dx, 0
        push    dx
        push    ax
        NEXT
_ABS8:
        mov     bx, sp
        and     byte [bx+7], 0x7F
        NEXT
;
; A real's sign changes, but not 0's: the PDP-11 has no -0.
;
_NEG8:
        mov     bx, sp
        mov     ax, [bx+6]
        and     ax, 0x7FFF
        or      ax, [bx+4]
        or      ax, [bx+2]
        or      ax, [bx]
        jz      .1
        xor     byte [bx+7], 0x80
.1:
        NEXT
