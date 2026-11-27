;
; CONOPS
;
; What the code has is pushed as it is there, its first word on top.
;
_CON1:
        push    cx
        NEXT
_CON2:
        lodsw
        push    ax
        NEXT
_CON4:
        lodsw
        mov     dx, ax
        lodsw
        push    ax
        push    dx
        NEXT
_CON8:
        mov     bx, si
        add     si, 8
        push    word [bx+6]
        push    word [bx+4]
        push    word [bx+2]
        push    word [bx]
        NEXT
_CON:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, si
        mov     ax, cx
        inc     ax
        and     al, 0xFE
        add     si, ax
        jmp     indcon
