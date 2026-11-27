; putw/putc -- write words/characters on output file
;
; The PDP-11's, which px and pxp have of the V6 library, for the 286,
; with the buffer of getc.s.  BP is the frame, as r5 was, and DI the
; buffer, as r1 was.  An error in writing is left in werflg.
;
; fflush is v6flush here: the C library's stdio has an fflush, in the
; same file as what its sprintf needs, which ctime calls.

        bits    16
        cpu     286
        segment _TEXT public align=2 class=CODE
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _DATA _BSS

        global  _putc, _putw, _v6flush, _fcreat
        global  _werflg
        extern  _creat, _write, _errno

        segment _TEXT
_fcreat:
        push    bp
        mov     bp, sp
        push    di
        mov     di, [bp+6]
        mov     word [di], -1           ; an illegal file desc.
        push    word 0x1A4              ; 0644
        push    word [bp+4]
        call    _creat
        add     sp, 4
        test    ax, ax
        js      badret
        mov     [di], ax
        mov     word [di+2], 0
        mov     word [di+4], 0
        jmp     goodret

_putw:
        push    bp
        mov     bp, sp
        push    di
        mov     di, [bp+6]
        dec     word [di+2]
        jge     .1
        call    fl
        dec     word [di+2]
.1:
        mov     bx, [di+4]
        mov     al, [bp+4]
        mov     [bx], al
        inc     word [di+4]
        dec     word [di+2]
        jge     .2
        call    fl
        dec     word [di+2]
.2:
        mov     bx, [di+4]
        mov     al, [bp+5]
        mov     [bx], al
        inc     word [di+4]
        mov     ax, [bp+4]
        jmp     goodret

_putc:
        push    bp
        mov     bp, sp
        push    di
        mov     di, [bp+6]
        dec     word [di+2]
        jge     .1
        call    fl
        dec     word [di+2]
.1:
        mov     ax, [bp+4]
        mov     bx, [di+4]
        mov     [bx], al
        inc     word [di+4]
        jmp     goodret

_v6flush:                               ; fflush
        push    bp
        mov     bp, sp
        push    di
        mov     di, [bp+4]
        call    fl
        jmp     goodret

fl:
        lea     ax, [di+6]
        push    ax
        cmp     word [di+4], 0
        je      .1
        mov     cx, [di+4]
        sub     cx, ax
        push    cx
        push    ax
        push    word [di]
        call    _write
        add     sp, 6
        test    ax, ax
        jns     .1
        mov     [_werflg], ax
.1:
        pop     word [di+4]
        mov     word [di+2], 512
        ret

badret:
        lea     sp, [bp-2]
        pop     di
        pop     bp
        ret

goodret:
        mov     word [_errno], 0
        pop     di
        pop     bp
        ret

        segment _BSS
_werflg: resw   1
