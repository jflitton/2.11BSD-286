;
; MULTIPLICATION & SQUARING
;
_SQR2:
        mov     bx, sp
        push    word [bx]
_MUL2:
        pop     ax
        pop     bx
        imul    bx
        push    dx
        push    ax
        NEXT
_MUL42:
        pop     ax
        pop     di                      ; the right
        pop     bx
        mov     cx, bx
        sar     cx, 15                  ; the left
        jmp     mul4
_MUL24:
        pop     ax
        cwd
        mov     di, dx                  ; the right
        pop     bx
        pop     cx                      ; the left
        jmp     mul4
_SQR4:
        pop     bx
        pop     cx
        mov     ax, bx
        mov     di, cx
        jmp     mul4
_MUL4:
        pop     ax
        pop     di
        pop     bx
        pop     cx
;
; mul4: the left CX:BX times the right DI:AX, its low 32 bits, which
; are the same signed or not
;
mul4:
        mov     bp, ax
        mov     ax, cx
        mul     bp
        mov     cx, ax
        mov     ax, di
        mul     bx
        add     cx, ax
        mov     ax, bp
        mul     bx
        add     dx, cx
        push    dx
        push    ax
        NEXT
_SQR8:
        mov     bx, sp
        fld     qword [bx]
        fmul    qword [bx]
        jmp     fput
_MUL8:
        mov     bx, sp
        fld     qword [bx+8]
        fmul    qword [bx]
        add     sp, 8
        jmp     fput
_MUL28:
        mov     bx, sp
        fld     qword [bx+2]
        fimul   word [bx]
        add     sp, 2
        jmp     fput
_MUL48:
        mov     bx, sp
        fld     qword [bx+4]
        fimul   dword [bx]
        add     sp, 4
        jmp     fput
_MUL82:
        mov     bx, sp
        fild    word [bx+8]
        fmul    qword [bx]
        add     sp, 2
        jmp     fput
_MUL84:
        mov     bx, sp
        fild    dword [bx+8]
        fmul    qword [bx]
        add     sp, 4
        jmp     fput
