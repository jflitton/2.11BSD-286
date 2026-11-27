;
; CASE OPERATORS
;
; The subop or the word after is how many labels there are; after it
; a table of as many offsets, and then the labels.  The offset of the
; label that is the value is added to where the table is.
;
_CASE1OP:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     di, si
        add     di, cx
        add     di, cx
        mov     dx, cx
        pop     ax
.2:
        cmp     al, [di]
        je      casefound
        inc     di
        loop    .2
        ERROR   ECASE
_CASE2OP:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     di, si
        add     di, cx
        add     di, cx
        mov     dx, cx
        pop     ax
.2:
        cmp     ax, [di]
        je      casefound
        add     di, 2
        loop    .2
        ERROR   ECASE
_CASE4OP:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     di, si
        add     di, cx
        add     di, cx
        mov     dx, cx
        pop     ax
        pop     bx
.2:
        cmp     ax, [di]
        jne     .3
        cmp     bx, [di+2]
        je      casefound
.3:
        add     di, 4
        loop    .2
        ERROR   ECASE
;
; casefound: DX labels, of which CX were left, this one among them
;
casefound:
        sub     dx, cx
        add     dx, dx
        mov     bx, si
        add     bx, dx
        add     si, [bx]
        NEXT
