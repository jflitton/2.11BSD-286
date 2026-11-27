;
; PXP stuff
;
; The counts are longs, as they are in memory, after a header of 12
; bytes: 0426, 0, the time, how many counts, and a word of the code's.
; As on the PDP-11 the size alloc() was given stays on the stack.
;
_PXPBUF:
        mov     ax, [si]
        add     ax, 3
        shl     ax, 2
        push    ax
        mov     [_pxpsize], ax
        call    _alloc
        mov     [_pxpbuf], ax
        mov     di, ax
        mov     word [di], 0x116        ; 0426
        mov     word [di+2], 0
        push    word 0
        call    _time
        add     sp, 2
        mov     [di+4], ax
        mov     [di+6], dx
        lodsw
        mov     [di+8], ax
        add     si, 2
        lodsw
        mov     [di+10], ax
        NEXT
_TRACNT:
        mov     cx, [si+4]
        add     si, [si]
        jmp     count
_COUNT:
        jnz     count
        lodsw
        mov     cx, ax
count:
        shl     cx, 2
        mov     bx, [_pxpbuf]
        add     bx, cx
        add     word [bx], 1
        adc     word [bx+2], 0
        NEXT
