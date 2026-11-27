;
; FORS
;
; The address of the variable on top, the limit, a long, under it.  The
; variable is stepped, and if what it was had not reached the limit
; the loop goes round again, by the offset in the code.
;
_FOR1U:
        pop     bx
        mov     al, [bx]
        cbw
        cwd
        inc     byte [bx]
        jmp     foru
_FOR2U:
        pop     bx
        mov     ax, [bx]
        cwd
        inc     word [bx]
        jmp     foru
_FOR4U:
        pop     bx
        mov     ax, [bx]
        mov     dx, [bx+2]
        add     word [bx], 1
        adc     word [bx+2], 0
foru:
        pop     bx
        pop     di
        cmp     dx, di
        je      .1
        jg      .2
        add     si, [si]
        NEXT
.1:
        cmp     ax, bx
        jae     .2
        add     si, [si]
        NEXT
.2:
        add     si, 2
        NEXT
_FOR1D:
        pop     bx
        mov     al, [bx]
        cbw
        cwd
        dec     byte [bx]
        jmp     ford
_FOR2D:
        pop     bx
        mov     ax, [bx]
        cwd
        dec     word [bx]
        jmp     ford
_FOR4D:
        pop     bx
        mov     ax, [bx]
        mov     dx, [bx+2]
        sub     word [bx], 1
        sbb     word [bx+2], 0
ford:
        pop     bx
        pop     di
        cmp     dx, di
        je      .1
        jl      .2
        add     si, [si]
        NEXT
.1:
        cmp     ax, bx
        jbe     .2
        add     si, [si]
        NEXT
.2:
        add     si, 2
        NEXT
