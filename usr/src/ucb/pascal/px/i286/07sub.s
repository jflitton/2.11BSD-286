;
; SUBTRACTION
;
_SUB42:
        pop     bx
        pop     di
        pop     ax
        cwd
        sub     ax, bx
        sbb     dx, di
        push    dx
        push    ax
        NEXT
_SUB2:
        pop     ax
        cwd
        mov     bx, ax
        mov     di, dx
        pop     ax
        cwd
        sub     ax, bx
        sbb     dx, di
        push    dx
        push    ax
        NEXT
_SUB24:
        pop     ax
        cwd
        mov     bx, sp
        sub     [bx], ax
        sbb     [bx+2], dx
        NEXT
_SUB4:
        pop     ax
        pop     dx
        mov     bx, sp
        sub     [bx], ax
        sbb     [bx+2], dx
        NEXT
_SUB8:
        mov     bx, sp
        fld     qword [bx+8]
        fsub    qword [bx]
        add     sp, 8
        jmp     fput
_SUB28:
        mov     bx, sp
        fld     qword [bx+2]
        fisub   word [bx]
        add     sp, 2
        jmp     fput
_SUB48:
        mov     bx, sp
        fld     qword [bx+4]
        fisub   dword [bx]
        add     sp, 4
        jmp     fput
_SUB82:
        mov     bx, sp
        fild    word [bx+8]
        fsub    qword [bx]
        add     sp, 2
        jmp     fput
_SUB84:
        mov     bx, sp
        fild    dword [bx+8]
        fsub    qword [bx]
        add     sp, 4
        jmp     fput
