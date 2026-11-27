        global  _preadn, _atol
;
; preadn(pfloat, pdigits, pbuf)
; char *pbuf;
;
; This routine reads a number from the current active file.
; pdigits is the size of pbuf in characters.
; pfloat is 0 to read an integer, 1 to read a real number.
; it returns with the carry flag set if it reaches EOF before
; reading any characters of the number while EOF was not true
; before the call.
;
; DI is pbuf, CX what is left of pdigits, DX the count of digits, and
; later what of a real has been read, AX the character, and BP the
; stack the arguments are on.
;
; The PDP-11's tested the file's flags from before the call, which had
; no EOF, for the end of the file after a digit: the last digit of a
; file that does not end in a newline was read again, and then the end
; was read past.  Here it is the file's flags now that are tested.
;
pfloat  equ     2
pdigits equ     4
pbuf    equ     6

_preadn:
        mov     bp, sp
        mov     di, [bp+pbuf]
        mov     cx, [bp+pdigits]
        call    iosync
        mov     bx, [buf]
        test    word [bx+FUNIT], EOF
        jnz     preof
        jmp     .2
;
; skip white space
;
.1:
        call    get
        call    iosync
        mov     bx, [buf]
        test    word [bx+FUNIT], EOF
        jnz     prsec
.2:
        mov     bx, [buf]
        mov     al, [bx]
        cmp     al, ' '
        je      .1
        cmp     al, 9
        je      .1
        cmp     al, 12
        je      .1
        cmp     al, 10
        je      .1
;
; starting point for
; a signed number
; record + and - if present
;
psignd:
        xor     dx, dx                  ; digit count
        cmp     al, '-'
        jne     .1
        dec     cx
        jle     prtood
        mov     [di], al
        inc     di
        jmp     .2
.1:
        cmp     al, '+'
        jne     pdigit
.2:
        call    get
        jmp     pdigit
;
; starting point for
; a unsigned number
;
punsd:
        xor     dx, dx
pdigit:
        call    iosync
        mov     bx, [buf]
        xor     ax, ax
        test    word [bx+FUNIT], EOF
        jnz     pnond
        mov     al, [bx]
        cmp     al, '0'
        jb      pnond
        cmp     al, '9'
        ja      pnond
        dec     cx
        jle     prtood
        mov     [di], al
        inc     di
        inc     dx
        call    get
        jmp     pdigit
;
; got a non digit or EOF
;
pnond:
        test    dx, dx
        jz      prerr
        mov     dx, [bp+pfloat]
        test    dx, dx
        jz      .4
        cmp     dx, 1
        jne     .1
        cmp     al, '.'
        je      .3
        inc     dx
.1:
        cmp     dx, 2
        jne     .4
        cmp     al, 'e'
        je      .3
        cmp     al, 'E'
        jne     .4
.3:
        dec     cx
        jle     prtood
        mov     [di], al
        inc     di
        call    get
        inc     dx
        mov     [bp+pfloat], dx
        sub     dx, 2
        jz      punsd
        call    iosync
        mov     bx, [buf]
        test    word [bx+FUNIT], EOF
        jnz     prbadf
        mov     al, [bx]
        jmp     psignd
.4:
        dec     cx
        jle     prtood
        mov     byte [di], 0
        inc     di
        clc
        ret
prsec:
        stc
        ret
prtood:
        ERROR   ETOODIGITS
prerr:
        cmp     word [bp+pfloat], 0
        je      prbadi
prbadf:
        ERROR   EBADFNUM
prbadi:
        ERROR   EBADINUM
preof:
        ERROR   EPASTEOF
;
; long atol(str)
; char *str;
;
; Convert string str to an integer
;
; DX:AX is the number, DI:CX twice it, and BX the string.
;
_atol:
        push    di
        mov     bx, sp
        mov     bx, [bx+4]
        push    bx
        xor     ax, ax
        xor     dx, dx
        cmp     byte [bx], '-'
        jne     .1
        inc     bx
.1:
        cmp     byte [bx], 0
        je      .2
        shl     ax, 1
        rcl     dx, 1
        jo      .9
        mov     cx, ax
        mov     di, dx
        shl     ax, 1
        rcl     dx, 1
        jo      .9
        shl     ax, 1
        rcl     dx, 1
        jo      .9
        add     ax, cx
        adc     dx, di
        jo      .9
        mov     cl, [bx]
        inc     bx
        sub     cl, '0'
        xor     ch, ch
        add     ax, cx
        adc     dx, 0
        jo      .9
        jmp     .1
.2:
        pop     bx
        cmp     byte [bx], '-'
        jne     .3
        neg     dx
        neg     ax
        sbb     dx, 0
.3:
        pop     di
        ret
.9:
        ERROR   EBIGGIE
