;
; range checking
;
; The value stays on top of the stack.  A bound of 4 bytes in the code
; is as a long is in memory, its low word first.
;
_RANG42:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, sp
        mov     ax, [bx]
        cwd
        cmp     dx, [bx+2]
        jne     rangerr
        jmp     rang2
_RANG2:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, sp
        mov     ax, [bx]
rang2:
        cmp     ax, cx
        jl      rangerr
        cmp     ax, [si]
        jg      rangerr
        add     si, 2
        NEXT
_RSNG42:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, sp
        mov     ax, [bx]
        cwd
        cmp     dx, [bx+2]
        jne     rangerr
        cmp     ax, cx
        ja      rangerr
        NEXT
_RSNG2:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bx, sp
        cmp     [bx], cx
        ja      rangerr
        NEXT
_RSNG4:
        mov     bx, sp
        mov     ax, [bx]
        mov     dx, [bx+2]
        test    dx, dx
        js      rangerr
        jmp     rang4hi
_RSNG24:
        mov     bx, sp
        mov     ax, [bx]
        test    ax, ax
        js      rangerr
        cwd
        jmp     rang4hi
_RANG24:
        mov     bx, sp
        mov     ax, [bx]
        cwd
        jmp     rang4
_RANG4:
        mov     bx, sp
        mov     ax, [bx]
        mov     dx, [bx+2]
;
; rang4: DX:AX at least the bound in the code, and then at most the next
;
rang4:
        cmp     dx, [si+2]
        jl      rangerr
        jg      .1
        cmp     ax, [si]
        jb      rangerr
.1:
        add     si, 4
rang4hi:
        cmp     dx, [si+2]
        jg      rangerr
        jl      .1
        cmp     ax, [si]
        ja      rangerr
.1:
        add     si, 4
        NEXT
rangerr:
        ERROR   ERANGE
