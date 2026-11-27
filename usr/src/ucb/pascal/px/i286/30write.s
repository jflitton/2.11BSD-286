        global  _pwril
;
; pwril(w, lint)
; int w;
; long lint;
;
; This routine writes the character representation of lint
; in a field at least w characters wide using pwrite.
;
; The PDP-11's divided by a table of the quotients of the high word;
; here the 286's division gives a digit for two divisions, into a
; buffer on the stack, the last first.  Each character is written by
; pputch, which is what pwrite does with O_WRITC.
;
_pwril:
        push    bp
        mov     bp, sp
        push    si
        push    di
        sub     sp, 12                  ; the digits, at most 10
        lea     di, [bp-4]              ; after the last
        mov     ax, [bp+6]
        mov     dx, [bp+8]
        xor     si, si                  ; 1 if it is negative
        test    dx, dx
        jns     .1
        neg     dx
        neg     ax
        sbb     dx, 0
        inc     si
.1:
        mov     cx, 10
.2:
        mov     bx, ax
        mov     ax, dx
        xor     dx, dx
        div     cx                      ; the high word
        xchg    ax, bx
        div     cx                      ; and the low, with its remainder
        add     dl, '0'
        dec     di
        mov     [di], dl
        mov     dx, bx
        or      bx, ax
        jnz     .2
        lea     cx, [bp-4]
        sub     cx, di                  ; the digits
        mov     bx, [bp+4]
        sub     bx, cx
        sub     bx, si                  ; the blanks before them
.3:
        dec     bx
        js      .4
        push    word ' '
        call    _pputch
        add     sp, 2
        jmp     .3
.4:
        test    si, si
        jz      .5
        push    word '-'
        call    _pputch
        add     sp, 2
.5:
        mov     al, [di]
        xor     ah, ah
        push    ax
        call    _pputch
        add     sp, 2
        inc     di
        loop    .5
        add     sp, 12
        pop     di
        pop     si
        pop     bp
        ret
