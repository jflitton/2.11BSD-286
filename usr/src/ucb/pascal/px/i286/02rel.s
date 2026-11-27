;
; RELATIONAL OPERATORS
;
; Each compares the left with the right and goes through reltab by the
; subop, with the flags as a signed comparison of the two leaves them.
;
_REL2:
        pop     ax
        pop     dx
        cmp     dx, ax
        mov     bx, cx
        jmp     [cs:bx+reltab]
_REL42:
        pop     bx
        pop     di
        pop     ax
        cwd
        jmp     rel4
_REL24:
        pop     ax
        cwd
        mov     bx, ax
        mov     di, dx
        pop     ax
        pop     dx
        jmp     rel4
_REL4:
        pop     bx
        pop     di
        pop     ax
        pop     dx
;
; rel4: the left DX:AX with the right DI:BX
;
rel4:
        cmp     dx, di
        jne     .1
        cmp     ax, bx
        je      .1
        mov     ax, 1
        jae     .2
        mov     ax, -1
.2:
        cmp     ax, 0
.1:
        mov     bx, cx
        jmp     [cs:bx+reltab]
_REL28:
        mov     bx, sp
        fld     qword [bx+2]
        ficomp  word [bx]
        add     sp, 10
        jmp     relf
_REL48:
        mov     bx, sp
        fld     qword [bx+4]
        ficomp  dword [bx]
        add     sp, 12
        jmp     relf
_REL82:
        mov     bx, sp
        fild    word [bx+8]
        fcomp   qword [bx]
        add     sp, 10
        jmp     relf
_REL84:
        mov     bx, sp
        fild    dword [bx+8]
        fcomp   qword [bx]
        add     sp, 12
        jmp     relf
_REL8:
        mov     bx, sp
        fld     qword [bx+8]
        fcomp   qword [bx]
        add     sp, 16
relf:
        call    fcc
        mov     bx, cx
        jmp     [cs:bx+reltab]
;
; fcc: the 287's comparison, which FSTSW and SAHF give as an unsigned
; one sets the flags, made as a signed one does, as the PDP-11's cfcc
; gives it
;
fcc:
        fstsw   ax
        sahf
        mov     ax, 0
        je      .1
        mov     ax, 1
        jae     .1
        mov     ax, -1
.1:
        cmp     ax, 0
        ret
_RELG:
        lodsw
        mov     dx, ax
        inc     ax
        and     al, 0xFE
        mov     di, sp
        mov     bx, di
        add     bx, ax
        add     ax, bx
        mov     bp, ax
.1:
        mov     al, [bx]
        cmp     al, [di]
        jne     .2
        inc     bx
        inc     di
        dec     dx
        jnz     .1
        cmp     ax, ax
.2:
        mov     sp, bp
        mov     bx, cx
        jmp     [cs:bx+reltab]

        segment _TEXT
reltab:
        dw      releq, relne, rellt, relgt, relle, relge
        dw      ifeq, ifne, iflt, ifgt, ifle, ifge

releq:
        je      true
        push    word 0
        NEXT
relne:
        jne     true
        push    word 0
        NEXT
rellt:
        jl      true
        push    word 0
        NEXT
relgt:
        jg      true
        push    word 0
        NEXT
relle:
        jle     true
        push    word 0
        NEXT
relge:
        jge     true
        push    word 0
        NEXT
true:
        push    word 1
        NEXT
false:
        push    word 0
        NEXT
ifeq:
        jne     iftra
        add     si, 2
        NEXT
ifne:
        je      iftra
        add     si, 2
        NEXT
iflt:
        jge     iftra
        add     si, 2
        NEXT
ifgt:
        jle     iftra
        add     si, 2
        NEXT
ifle:
        jg      iftra
        add     si, 2
        NEXT
ifge:
        jl      iftra
        add     si, 2
        NEXT
iftra:
        add     si, [si]
        NEXT
