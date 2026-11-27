;
; MODULO
;
; The remainder has the sign of the left, as the PDP-11's has.  Of longs
; it is the 287's FPREM, which is exact.
;
_MOD42:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      modchk
        fild    dword [bx]
        fild    word [bx+4]
        add     sp, 2
        jmp     mod4
_MOD24:
        mov     bx, sp
        cmp     word [bx], 0
        je      modchk
        fild    word [bx]
        fild    dword [bx+2]
        add     sp, 2
        jmp     mod4
_MOD4:
        mov     bx, sp
        mov     ax, [bx]
        or      ax, [bx+2]
        jz      modchk
        fild    dword [bx]
        fild    dword [bx+4]
        add     sp, 4
;
; mod4: the left on top of the 287's stack and the right under it
;
mod4:
        fprem
        fstsw   ax
        sahf
        jp      mod4
        fstp    st1
        mov     bx, sp
        fistp   dword [bx]
        fwait
        NEXT
_MOD2:
        pop     bx
        test    bx, bx
        jz      modchk
        pop     ax
        cwd
        cmp     bx, -1
        je      .1
        idiv    bx
        mov     ax, dx
        cwd
        push    dx
        push    ax
        NEXT
.1:
        push    word 0
        push    word 0
        NEXT
modchk:
        ERROR   EMODCHK
