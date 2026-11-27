;
; RVALUES
;
_RV2:
        mov     bx, cx
        mov     bx, [bx+_display]
        lodsw
        add     bx, ax
        push    word [bx]
        NEXT
_RV4:
        mov     bx, cx
        mov     bx, [bx+_display]
        lodsw
        add     bx, ax
        push    word [bx+2]
        push    word [bx]
        NEXT
_RV1:
        mov     bx, cx
        mov     bx, [bx+_display]
        lodsw
        add     bx, ax
        mov     al, [bx]
        cbw
        push    ax
        NEXT
_RV8:
        mov     bx, cx
        mov     bx, [bx+_display]
        lodsw
        add     bx, ax
        push    word [bx+6]
        push    word [bx+4]
        push    word [bx+2]
        push    word [bx]
        NEXT
_RV:
        mov     bx, cx
        mov     bx, [bx+_display]
        lodsw
        add     bx, ax
        lodsw
        mov     cx, ax
        jmp     indcon
