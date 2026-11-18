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
        ; Shift and subtract, 16 steps, remainder in SI:DI.
        push    si
        push    di
        push    bp
        mov     di, dx          ; remainder = hi(n)
        xor     si, si
        mov     dx, ax          ; the 16 bits still to bring down
        xor     ax, ax          ; quotient
        mov     bp, 16
.loop:  shl     ax, 1
        shl     dx, 1
        rcl     di, 1
        rcl     si, 1
        cmp     si, cx
        ja      .sub
        jb      .next
        cmp     di, bx
        jb      .next
.sub:   sub     di, bx
        sbb     si, cx
        inc     ax
.next:  dec     bp
        jnz     .loop
        mov     bx, di
        mov     cx, si
        xor     dx, dx
        pop     bp
        pop     di
        pop     si
        ret
