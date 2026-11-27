;
; INDS
;
_IND2:
        pop     bx
        push    word [bx]
        NEXT
_IND4:
        pop     bx
        push    word [bx+2]
        push    word [bx]
        NEXT
_IND1:
        pop     bx
        mov     al, [bx]
        cbw
        push    ax
        NEXT
_IND8:
        mov     cx, 8
        pop     bx
        jmp     indcon
_IND:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        pop     bx
;
; indcon: the CX bytes at BX pushed, made even with a 0, as they are
; in memory
;
indcon:
        add     bx, cx
        shr     cx, 1
        jnc     .2
        dec     bx
        mov     al, [bx]
        xor     ah, ah
        push    ax                      ; clean for sets, e.g.
.2:
        jcxz    .4
.3:
        dec     bx
        dec     bx
        push    word [bx]
        loop    .3
.4:
        NEXT
