;
; BOOLEAN OPERATIONS
;
_AND:
        pop     ax
        test    ax, ax
        jnz     .1
        mov     bx, sp
        mov     word [bx], 0
.1:
        NEXT
_OR:
        pop     ax
        mov     bx, sp
        or      [bx], ax
        NEXT
_NOT:
        mov     bx, sp
        inc     word [bx]
        and     word [bx], 1
        NEXT
