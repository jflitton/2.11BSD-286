; getw/getc -- get words/characters from input file
; fopen -- open a file for use by get(c|w)
;
; The PDP-11's, which px and pxp have of the V6 library, for the 286.
; The buffer is the descriptor, the count of what is left, the next
; character, and 512 bytes.  BP is the frame, as r5 was, and DI the
; buffer, as r1 was; at the end of the file, or an error, what read()
; gave, which is negative, is given back.

        bits    16
        cpu     286
        segment _TEXT public align=2 class=CODE
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _DATA _BSS

        global  _getc, _getw, _fopen
        extern  _open, _read, _errno

        segment _TEXT
_fopen:
        push    bp
        mov     bp, sp
        push    di
        push    word 0
        push    word [bp+4]
        call    _open
        add     sp, 4
        test    ax, ax
        js      badret
        mov     di, [bp+6]
        mov     [di], ax
        mov     word [di+2], 0
        xor     ax, ax
        pop     di
        pop     bp
        ret

_getw:
        mov     word [_errno], 0
        push    bp
        mov     bp, sp
        push    di
        mov     di, [bp+4]
        sub     word [di+2], 2
        jge     .1
        cmp     word [di+2], -1
        jl      .2
        mov     bx, [di+4]
        mov     al, [bx]                ; the one left
        push    ax
        call    fill
        pop     ax
        mov     bx, [di+4]
        jmp     .3
.2:
        call    fill
        dec     word [di+2]
.1:
        mov     bx, [di+4]
        mov     al, [bx]
        inc     bx
.3:
        mov     ah, [bx]
        inc     bx
        mov     [di+4], bx
        pop     di
        pop     bp
        ret

_getc:
        push    bp
        mov     bp, sp
        push    di
        mov     di, [bp+4]
        dec     word [di+2]
        jge     .1
        call    fill
.1:
        mov     bx, [di+4]
        xor     ax, ax
        mov     al, [bx]
        inc     word [di+4]
        pop     di
        pop     bp
        ret

fill:
        lea     ax, [di+6]
        mov     [di+4], ax
        push    word 512
        push    ax
        push    word [di]
        call    _read
        add     sp, 6
        test    ax, ax
        js      badret
        dec     ax
        js      badret
        mov     [di+2], ax
        ret

badret:
        lea     sp, [bp-2]
        pop     di
        pop     bp
        ret
