;
; INTEGER DIVISION
;
; Of longs it is the 287's, as on the PDP-11 its FPU's, made an integer
; toward zero: a quotient of two longs is exact in its 64 bits.  The
; PDP-11's DIV2 gave -32768 for -32768 div -1; here it is 32768.
;
_DIV42:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      divchk
        fild    word [bx+4]
        fidiv   dword [bx]
        add     sp, 2
        jmp     fistl
_DIV24:
        mov     bx, sp
        cmp     word [bx], 0
        je      divchk
        fild    dword [bx+2]
        fidiv   word [bx]
        add     sp, 2
        jmp     fistl
_DIV4:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      divchk
        fild    dword [bx+4]
        fidiv   dword [bx]
        add     sp, 4
;
; fistl: the top of the 287's stack, toward zero, a long on top of
; this one, where there are 4 bytes for it
;
fistl:
        call    __CHP
        mov     bx, sp
        fistp   dword [bx]
        fwait
        NEXT
_DIV2:
        pop     bx
        test    bx, bx
        jz      divchk
        pop     ax
        cwd
        cmp     bx, -1
        je      .1
        idiv    bx
        cwd
        push    dx
        push    ax
        NEXT
.1:
        neg     dx
        neg     ax
        sbb     dx, 0
        push    dx
        push    ax
        NEXT
divchk:
        ERROR   EDIVCHK
