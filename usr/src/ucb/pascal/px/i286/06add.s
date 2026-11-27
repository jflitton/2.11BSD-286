;
; ADDITION
;
; An integer result is a long; of a real, the 287's result is put on
; top of the stack by fput, from the place the operands took.
;
_ADD42:
        pop     bx
        pop     di
        pop     ax
        cwd
        add     ax, bx
        adc     dx, di
        push    dx
        push    ax
        NEXT
_ADD2:
        pop     ax
        cwd
        mov     bx, ax
        mov     di, dx
        pop     ax
        cwd
        add     ax, bx
        adc     dx, di
        push    dx
        push    ax
        NEXT
_ADD24:
        pop     ax
        cwd
        mov     bx, sp
        add     [bx], ax
        adc     [bx+2], dx
        NEXT
_ADD4:
        pop     ax
        pop     dx
        mov     bx, sp
        add     [bx], ax
        adc     [bx+2], dx
        NEXT
_ADD8:
        mov     bx, sp
        fld     qword [bx+8]
        fadd    qword [bx]
        add     sp, 8
        jmp     fput
_ADD28:
        mov     bx, sp
        fld     qword [bx+2]
        fiadd   word [bx]
        add     sp, 2
        jmp     fput
_ADD48:
        mov     bx, sp
        fld     qword [bx+4]
        fiadd   dword [bx]
        add     sp, 4
        jmp     fput
_ADD82:
        mov     bx, sp
        fild    word [bx+8]
        fadd    qword [bx]
        add     sp, 2
        jmp     fput
_ADD84:
        mov     bx, sp
        fild    dword [bx+8]
        fadd    qword [bx]
        add     sp, 4
;
; fput: the real at the top of the 287's stack to the top of this one,
; where there are 8 bytes for it.  Too large for a double, it is the
; error the PDP-11's overflow is.
;
fput:
        mov     bx, sp
        fstp    qword [bx]
        fwait
        mov     ax, [bx+6]
        and     ax, 0x7FF0
        cmp     ax, 0x7FF0
        je      fpovflo
        NEXT
fpovflo:
        ERROR   EFPOVFLO
