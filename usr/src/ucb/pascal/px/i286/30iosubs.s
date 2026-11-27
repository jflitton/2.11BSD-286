;
; IO SUBROUTINES
;
; Each keeps every register but AX.
;
        global  _pmessage, _pputch, _draino, _unit0, _unit1, _unit2

;
; pmessage(): what is written goes to the message file now, and every
; file written is flushed
;
_pmessage:
        push    bx
        push    di
        mov     di, [_file]
        mov     ax, _unit2
        call    unit
        mov     [_file], di
        mov     di, [fchain]            ; flush all files
.1:
        test    di, di
        jz      .2
        test    word [di+FUNIT], FWRITE
        jz      .3
        push    word [di+FBUF]
        call    _v6flush
        add     sp, 2
.3:
        mov     di, [di+FCHAIN]
        jmp     .1
.2:
        push    word [_unit1+FBUF]
        call    _v6flush
        add     sp, 2
        pop     di
        pop     bx
        ret
;
; pputch(c): c to the current file
;
_pputch:
        push    bx
        mov     bx, sp
        mov     ax, [bx+4]
        mov     bx, [buf]
        mov     [bx], ax
        call    put
        pop     bx
        ret
;
; iosync insures that
; a useable image is in
; the buffer window.
;
iosync:
        push    bx
        mov     bx, [buf]
        test    word [bx+FUNIT], SYNC   ; dirty bit
        jnz     .1
        pop     bx
        ret
.1:
        push    cx
        push    dx
        push    di
        mov     di, bx
        and     word [bx+FUNIT], 0xFFFF ^ SYNC
        test    word [bx+FUNIT], EOF
        jnz     pasteof
        mov     cx, [bx+FSIZE]
.2:
        mov     bx, [buf]
        push    cx
        push    word [bx+FBUF]
        call    _getc
        add     sp, 2
        pop     cx
        test    ax, ax
        js      .4
        mov     [di], al
        inc     di
        loop    .2
        mov     bx, [buf]
        and     word [bx+FUNIT], 0xFFFF ^ EOLN
        test    word [bx+FUNIT], FTEXT
        jz      .3
        cmp     byte [bx], 10
        jne     .3
        or      word [bx+FUNIT], EOLN
        mov     byte [bx], ' '
.3:
        pop     di
        pop     dx
        pop     cx
        pop     bx
        ret
.4:
        mov     bx, [buf]
        or      word [bx+FUNIT], EOF
        jmp     .3
pasteof:
        ERROR   EPASTEOF
;
; get insures that
; something is in the window
; and then sets the dirty
; bit effectively throwing the
; windowed data away
;
get:
        push    bx
        mov     bx, [buf]
        test    word [bx+FUNIT], FREAD
        jz      .9
        call    iosync
        or      word [bx+FUNIT], SYNC
        pop     bx
        ret
.9:
        ERROR   EREADIT
;
; put: the window to the file
;
put:
        push    bx
        push    cx
        push    dx
        push    di
        cmp     word [buf], _unit2
        je      .6
        mov     di, [buf]
        test    word [di+FUNIT], FWRITE
        jz      .8
        mov     cx, [di+FSIZE]
.1:
        mov     bx, [buf]
        push    cx
        push    word [bx+FBUF]          ; the hidden buffer
        mov     al, [di]
        push    ax                      ; the character to write
        call    _putc
        add     sp, 4
        pop     cx
        cmp     word [_errno], 0        ; error occurred?
        jne     .9
        inc     di
        loop    .1
.2:
        pop     di
        pop     dx
        pop     cx
        pop     bx
        ret
.6:
        push    word 1
        push    word _unit2
        push    word 2
        call    _write
        add     sp, 6
        test    ax, ax
        js      .9
        cmp     ax, 1
        jne     .9
        jmp     .2
.8:
        ERROR   EWRITEIT
.9:
        ERROR   EWRITE
;
; unit: the file whose window AX is becomes the current one
;
unit:
        mov     [buf], ax
        test    ax, ax
        jz      .1
        push    bx
        mov     bx, ax
        cmp     word [bx+FUNIT], 0
        jl      .1
        mov     ax, [bx+PFNAME]
        mov     [_file], ax
        pop     bx
        ret
.1:
        ERROR   EREFINAF
;
; standard files
;
        segment _DATA
;
; unit 0 for "input"
;
u0buf:  times FBUFSZ db 0
        dw      u0buf                   ; buffer pointer
        dw      0
        dw      0
        dw      stdin
        dw      0                       ; file name
        dw      FTEXT+FREAD+SYNC+0      ; flags & unit number
        dw      1                       ; size of items
_unit0:
        dw      0                       ; window
;
; unit 1 for "output"
;
u1buf:  dw      1                       ; file descriptor for putc
_draino:
u1cnt:  times FBUFSZ-2 db 0
        dw      u1buf                   ; putc buffer pointer
        dw      0
        dw      0
        dw      stdout
        dw      0
        dw      FTEXT+FWRITE+1+EOF
        dw      1
_unit1:
        dw      0
;
; unit2 for <message>
;
        dw      0
        dw      0
        dw      0
        dw      mesgf
        dw      0
        dw      FTEXT+FWRITE+2
        dw      1
_unit2:
        dw      0
;
nextu:  dw      '0'                     ; next temp file
tnam:   db      "tmp.x", 0
        align   2
bufopt: dw      1
stdin:  db      "standard input", 0
stdout: db      "standard output", 0
mesgf:  db      "message file", 0
        align   2

        segment _BSS
buf:    resw    1
fchain: resw    1

        segment _TEXT
