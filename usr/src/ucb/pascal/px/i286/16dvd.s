;
; FLOATING DIVISION
;
; The right is looked at for 0 first, an integer or a real.  The result,
; 8 bytes, is where the operands began.
;
_DVD2:
        mov     bx, sp
        cmp     word [bx], 0
        je      fdivchk
        fild    word [bx+2]
        fidiv   word [bx]
        sub     sp, 4
        jmp     fput
_DVD42:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      fdivchk
        fild    word [bx+4]
        fidiv   dword [bx]
        sub     sp, 2
        jmp     fput
_DVD82:
        call    fzero
        jz      fdivchk
        fild    word [bx+8]
        fdiv    qword [bx]
        add     sp, 2
        jmp     fput
_DVD24:
        mov     bx, sp
        cmp     word [bx], 0
        je      fdivchk
        fild    dword [bx+2]
        fidiv   word [bx]
        sub     sp, 2
        jmp     fput
_DVD4:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      fdivchk
        fild    dword [bx+4]
        fidiv   dword [bx]
        jmp     fput
_DVD84:
        call    fzero
        jz      fdivchk
        fild    dword [bx+8]
        fdiv    qword [bx]
        add     sp, 4
        jmp     fput
_DVD28:
        mov     bx, sp
        cmp     word [bx], 0
        je      fdivchk
        fld     qword [bx+2]
        fidiv   word [bx]
        add     sp, 2
        jmp     fput
_DVD48:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      fdivchk
        fld     qword [bx+4]
        fidiv   dword [bx]
        add     sp, 4
        jmp     fput
_DVD8:
        call    fzero
        jz      fdivchk
        fld     qword [bx+8]
        fdiv    qword [bx]
        add     sp, 8
        jmp     fput
fdivchk:
        ERROR   EFDIVCHK
;
; fzero: BX the top of the stack, and ZF whether the real there is 0
;
fzero:
        mov     bx, sp
        add     bx, 2
        mov     ax, [bx+6]
        and     ax, 0x7FFF
        or      ax, [bx+4]
        or      ax, [bx+2]
        or      ax, [bx]
        ret
