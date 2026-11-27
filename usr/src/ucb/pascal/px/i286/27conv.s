;
; CONVERSIONS
;
_STOI:
        pop     ax
        cwd
        push    dx
        push    ax
        NEXT
_STOD:
        mov     bx, sp
        fild    word [bx]
        sub     sp, 6
        mov     bx, sp
        fstp    qword [bx]
        fwait
        NEXT
_ITOD:
        mov     bx, sp
        fild    dword [bx]
        sub     sp, 4
        mov     bx, sp
        fstp    qword [bx]
        fwait
        NEXT
_ITOS:
        pop     ax
        pop     dx
        push    ax
        NEXT
