; __I4M, __U4M: 32-bit multiply.  The compiler calls these for `long * long`.
;
;       DX:AX = DX:AX * CX:BX          (low 32 bits; the same for signed
;                                        and unsigned)
; All other registers are preserved.

%include "DEFS.inc"

ENTRY __U4M
ENTRY __I4M
        push    si
        push    di
        mov     si, ax          ; si = lo(a)
        mov     di, dx          ; di = hi(a)
        mul     cx              ; ax = lo(lo(a) * hi(b))
        xchg    ax, di          ; di = that, ax = hi(a)
        mul     bx              ; ax = lo(hi(a) * lo(b))
        add     di, ax          ; di = both cross terms
        mov     ax, si
        mul     bx              ; dx:ax = lo(a) * lo(b)
        add     dx, di
        pop     di
        pop     si
        RET
