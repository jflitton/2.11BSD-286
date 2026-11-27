;
; RELATIONALS ON SETS
;
; The left set and the right, each of the size in the code, on the
; stack.  A subop past 10 is one of the IF's, which goes on or jumps;
; the others push true or false.  The PDP-11's had lp return from the
; comparison into the IF; here the comparison is a routine.
;
_RELT:
        lodsw
        inc     ax
        and     al, 0xFE
        mov     di, sp                  ; the right
        mov     bp, sp
        add     bp, ax                  ; the left
        mov     dx, bp
        add     dx, ax                  ; the stack after them
        shr     ax, 1
        mov     bx, cx
        mov     cx, ax
        cmp     bx, 10
        jg      .1
        call    [cs:bx+settab]
        mov     sp, dx
        push    ax
        NEXT
.1:
        call    [cs:bx+settab-12]
        mov     sp, dx
        test    ax, ax
        jz      .2
        add     si, 2
        NEXT
.2:
        add     si, [si]
        NEXT

        segment _TEXT
settab:
        dw      seteq, setne, setlt, setgt, setle, setge
;
; Each has the left at BP, the right at DI and their words in CX, and
; returns true or false in AX, keeping DX.
;
seteq:
        mov     ax, [bp]
        cmp     ax, [di]
        jne     setf
        add     bp, 2
        add     di, 2
        loop    seteq
        jmp     sett
setne:
        mov     ax, [bp]
        cmp     ax, [di]
        jne     sett
        add     bp, 2
        add     di, 2
        loop    setne
        jmp     setf
setlt:
        xor     bx, bx                  ; whether they differ
.1:
        mov     ax, [di]
        not     ax
        and     ax, [bp]
        jnz     setf
        mov     ax, [bp]
        xor     ax, [di]
        or      bx, ax
        add     bp, 2
        add     di, 2
        loop    .1
        test    bx, bx
        jz      setf
        jmp     sett
setgt:
        xor     bx, bx
.1:
        mov     ax, [bp]
        not     ax
        and     ax, [di]
        jnz     setf
        mov     ax, [bp]
        xor     ax, [di]
        or      bx, ax
        add     bp, 2
        add     di, 2
        loop    .1
        test    bx, bx
        jz      setf
        jmp     sett
setle:
        mov     ax, [di]
        not     ax
        and     ax, [bp]
        jnz     setf
        add     bp, 2
        add     di, 2
        loop    setle
        jmp     sett
setge:
        mov     ax, [bp]
        not     ax
        and     ax, [di]
        jnz     setf
        add     bp, 2
        add     di, 2
        loop    setge
sett:
        mov     ax, 1
        ret
setf:
        xor     ax, ax
        ret
