; __PIA, __PIS, __PTS and __PTC: arithmetic on huge pointers (NASM), for
; the large model.  The compiler calls these for a pointer declared
; __huge, which is to an object of more than 64K: segments of 64K one
; after another, whose selectors go up by 8 (segalloc(2)).  So a carry
; out of the offset is 8 more in the selector, where real-mode code would
; add 1000h to a segment.
;
;       __PIA   DX:AX = the pointer DX:AX and the long CX:BX
;       __PIS   DX:AX = the pointer DX:AX less the long CX:BX
;       __PTS   DX:AX = the pointer DX:AX less the pointer CX:BX, a long
;       __PTC   the flags of comparing the pointer DX:AX with CX:BX, as
;               of unsigned numbers
;
; No other register is changed.

%include "DEFS.inc"

ENTRY __PIS
        push    bx
        push    cx
        not     cx              ; CX:BX = -CX:BX
        neg     bx
        sbb     cx, -1
        jmp     short add

ENTRY __PIA
        push    bx
        push    cx
add:    add     ax, bx
        adc     cx, 0           ; the segments to go up, or down
        shl     cx, 3
        add     dx, cx
        pop     cx
        pop     bx
        RET

ENTRY __PTS
        push    cx
        sub     dx, cx          ; the segments apart: selectors differ by
        sar     dx, 3           ;   8 for each
        sub     ax, bx
        sbb     dx, 0
        pop     cx
        RET

ENTRY __PTC
        cmp     dx, cx
        jne     .done
        cmp     ax, bx
.done:  RET
