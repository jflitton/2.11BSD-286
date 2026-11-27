;
; OFF, INDEX, LV and NIL
;
_OFF:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, sp
        add     [bx], cx
        NEXT
_INX4:
        pop     ax                      ; should check bounds here
        mov     bx, sp
        mov     [bx], ax
        test    cx, cx
_INX2:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        pop     dx
        lodsw
        sub     dx, ax
        js      .9
        lodsw
        cmp     dx, ax
        jg      .9
        mov     ax, dx
        imul    cx
        mov     bx, sp
        add     [bx], ax
        NEXT
.9:
        ERROR   ESUBSCR
_NIL:
        mov     bx, sp
        cmp     word [bx], 0
        je      .9
        NEXT
.9:
        ERROR   ENILPTR
_LV:
        mov     bx, cx
        lodsw
        add     ax, [bx+_display]
        push    ax
        NEXT
_INX4P2:
        pop     ax
        mov     bx, sp
        mov     [bx], ax
_INX2P2:
        pop     ax
        sub     ax, [si]
        add     si, 2
        shl     ax, cl
        mov     bx, sp
        add     [bx], ax
        NEXT
