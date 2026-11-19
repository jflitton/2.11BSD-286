; __U4D, __I4D: 32-bit divide.  The compiler calls these for `/` and `%`
; on longs.
;
;       DX:AX = DX:AX / CX:BX          quotient
;       CX:BX = DX:AX % CX:BX          remainder
;
; Signed division truncates toward zero and the remainder takes the sign of
; the dividend, as the 286's own IDIV does.  Division by zero raises the
; processor's divide error.  SI, DI and BP are preserved.

%include "DEFS.inc"

ENTRY __U4D
        call    udiv
        RET

ENTRY __I4D
        push    si
        xor     si, si          ; bit 0: negate quotient; bit 1: negate remainder
        or      dx, dx
        jns     .1
        neg     dx              ; dividend = -dividend
        neg     ax
        sbb     dx, 0
        mov     si, 3
.1:     or      cx, cx
        jns     .2
        neg     cx              ; divisor = -divisor
        neg     bx
        sbb     cx, 0
        xor     si, 1
.2:     call    udiv
        test    si, 1
        jz      .3
        neg     dx
        neg     ax
        sbb     dx, 0
.3:     test    si, 2
        jz      .4
        neg     cx
        neg     bx
        sbb     cx, 0
.4:     pop     si
        RET

; Unsigned DX:AX / CX:BX, results as above.  Near, for the two entries.
udiv:
        or      cx, cx
        jnz     .big
        ; The divisor fits in 16 bits: two hardware divides.
        xchg    ax, cx          ; cx = lo(n), ax = 0
        xchg    ax, dx          ; ax = hi(n), dx = 0
        div     bx              ; ax = hi(q), dx = remainder so far
        xchg    ax, cx          ; ax = lo(n), cx = hi(q)
        div     bx              ; ax = lo(q), dx = remainder
        mov     bx, dx
        mov     dx, cx
        xor     cx, cx
        ret
.big:   ; The divisor is 65536 or more, so the quotient fits in 16 bits.
        ; Shift the divisor right until it does fit in 16, and the dividend
        ; by as much: one hardware divide of those gives the quotient or
        ; one more than it, never less, because the bits shifted out of
        ; the divisor could only have made it larger.  Multiply back to
        ; see which, and for the remainder.  (Sixteen steps of shifting
        ; and subtracting, which this was, took four times as long for a
        ; divisor of 17 bits.)
        push    si
        push    di
        push    bp
        push    cx              ; [bp+2]: the divisor's high word,
        push    bx              ; [bp]:   and its low
        mov     bp, sp
        mov     si, ax          ; the dividend, in DI:SI
        mov     di, dx
.shift: shr     dx, 1
        rcr     ax, 1
        shr     cx, 1
        rcr     bx, 1           ; (leaves the zero flag as SHR CX set it)
        jnz     .shift
        div     bx
        mov     bx, ax          ; the quotient, or one more
.back:  mov     ax, bx
        mul     word [bp+2]
        jc      .over           ; the product is beyond 32 bits
        mov     cx, ax
        mov     ax, bx
        mul     word [bp]
        add     dx, cx          ; DX:AX = quotient * divisor
        jc      .over
        cmp     dx, di
        ja      .over
        jb      .fits
        cmp     ax, si
        jbe     .fits
.over:  dec     bx              ; one too many
        jmp     .back
.fits:  sub     si, ax
        sbb     di, dx
        mov     ax, bx
        xor     dx, dx
        mov     bx, si
        mov     cx, di
        add     sp, 4
        pop     bp
        pop     di
        pop     si
        ret
