;
; SET OPERATIONS
;
; The right set on top, the left under it, which becomes the result.
;
_ADDT:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        inc     cx
        and     cl, 0xFE
        mov     di, sp
        mov     bx, sp
        add     bx, cx
        shr     cx, 1
.2:
        mov     ax, [di]
        or      [bx], ax
        add     di, 2
        add     bx, 2
        loop    .2
        mov     sp, di
        NEXT
_SUBT:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        inc     cx
        and     cl, 0xFE
        mov     di, sp
        mov     bx, sp
        add     bx, cx
        shr     cx, 1
.2:
        mov     ax, [di]
        not     ax
        and     [bx], ax
        add     di, 2
        add     bx, 2
        loop    .2
        lodsw
        not     ax
        and     [bx-2], ax
        mov     sp, di
        NEXT
_MULT:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        inc     cx
        and     cl, 0xFE
        mov     di, sp
        mov     bx, sp
        add     bx, cx
        shr     cx, 1
.2:
        mov     ax, [di]
        and     [bx], ax
        add     di, 2
        add     bx, 2
        loop    .2
        mov     sp, di
        NEXT
_CARD:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, sp                  ; the set
        mov     bp, cx
        inc     bp
        and     bp, 0xFFFE
        add     bp, sp                  ; the stack after it
        xor     dx, dx                  ; how many
.2:
        mov     al, [bx]
        xor     ah, ah
        mov     di, ax
        and     di, 15
        shl     di, 1
        add     dx, [cs:di+poptab]
        mov     di, ax
        shr     di, 3
        and     di, 0x1E
        add     dx, [cs:di+poptab]
        inc     bx
        loop    .2
        mov     sp, bp
        push    dx
        NEXT

        segment _TEXT
poptab:
        dw      0, 1, 1, 2, 1, 2, 2, 3          ; 0000 to 0111
        dw      1, 2, 2, 3, 2, 3, 3, 4          ; 1000 to 1111

_CTTOT:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        push    cx
        lodsw
        push    ax
        lodsw
        push    ax
        call    _pcttot
        mov     sp, ax
        NEXT
_IN:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        inc     cx
        and     cl, 0xFE                ; the set's bytes
        pop     ax                      ; what may be in it
        sub     ax, [si]
        js      .9
        cmp     ax, [si+2]
        jg      .9
        add     si, 4
        mov     bx, ax
        shr     bx, 3
        add     bx, sp
        mov     dx, cx
        mov     cl, al
        and     cl, 7
        mov     al, 1
        shl     al, cl
        test    [bx], al
        mov     ax, 0
        jz      .2
        inc     ax
.2:
        add     sp, dx
        push    ax
        NEXT
.9:
        add     si, 4
        add     sp, cx
        push    word 0
        NEXT
_INCT:
        pop     dx
        pop     cx
.1:
        pop     ax
        cmp     dx, ax
        jl      .2
        pop     ax
        cmp     dx, ax
        jg      .3
        dec     cx
        shl     cx, 2
        add     sp, cx
        push    word 1
        NEXT
.2:
        add     sp, 2
.3:
        loop    .1
        push    word 0
        NEXT
