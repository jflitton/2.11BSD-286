;
; ASSIGNMENT OPERATORS
;
; The value on top, and the address it goes to under it.
;
_AS42:
        pop     ax
        pop     dx
        jmp     as2
_AS2:
        pop     ax
as2:
        pop     bx
        mov     [bx], ax
        NEXT
_AS4:
        pop     ax
        pop     dx
as4:
        pop     bx
        mov     [bx], ax
        mov     [bx+2], dx
        NEXT
_AS24:
        pop     ax
        cwd
        jmp     as4
_AS41:
        pop     ax
        pop     dx
        jmp     as1
_AS21:
        pop     ax
as1:
        pop     bx
        mov     [bx], al
        NEXT
_AS28:
        mov     bx, sp
        fild    word [bx]
        add     sp, 2
        jmp     as8f
_AS48:
        mov     bx, sp
        fild    dword [bx]
        add     sp, 4
as8f:
        pop     bx
        fstp    qword [bx]
        fwait
        NEXT
_AS8:
        mov     cx, 8
        jmp     ascx
_AS:
        jnz     ascx
        lodsw
        mov     cx, ax
;
; ascx: CX bytes, each word of them on the stack, to the address under
; them
;
ascx:
        mov     ax, cx
        inc     ax
        and     al, 0xFE
        mov     bx, sp
        add     bx, ax
        mov     di, [bx]
        lea     bp, [bx+2]
        mov     dx, si
        push    ds
        pop     es
        mov     si, sp
        shr     cx, 1
        rep     movsw
        adc     cx, cx
        rep     movsb
        mov     si, dx
        mov     sp, bp
        NEXT
