;
; Berkeley Pascal Assembler Kernel, for the 286
;
        global  _interpret, _pmflush, _stcnt, _stlim, _pxpbuf, _pxpsize

        segment _TEXT
;
; interpret(code, size): from the code int.c has read, which it never
; returns from.  The PDP-11's had the emulator trap call error() and
; illegal instructions dump core; here neither is a signal.
;
_interpret:
        cld
        mov     word [_dp], _display
        mov     bx, sp
        mov     si, [bx+2]
        mov     [_display], sp
        push    word _unit0
        push    word _unit1
;
; main interpreter loop
; NEXT transfers here
;
loop:
        lodsb
        cbw
        add     ax, ax
        mov     bx, ax
        lodsb
        cbw
        mov     cx, ax
        test    cx, cx
        jmp     [cs:bx+optab]
badop:
        ERROR   EBADOP

        segment _BSS
_perrno: resw   1

        segment _TEXT
onemt:
        push    word [_perrno]
        call    _error                  ; which does not return
;
_ABORT:
        call    _abort
_HALT:
        ERROR   EHALT
_NODUMP:
        inc     word [_nodump]
_BEG:
        lodsw
        mov     cx, ax                  ; the locals' size, less than 0
        mov     bx, [_dp]
        add     bx, 2
        mov     [_dp], bx
        push    word [bx]
        push    si
        add     si, 10
        push    word [buf]
        push    word [_file]
        sub     sp, 2
        mov     [bx], sp
        mov     ax, sp
        add     ax, cx
        cmp     ax, sp
        ja      .9
        cmp     ax, [_maxstk]
        jbe     .9
        jcxz    .2
        mov     di, ax                  ; the locals, cleared
        neg     cx
        shr     cx, 1
        mov     sp, ax
        push    ds
        pop     es
        xor     ax, ax
        rep     stosw
.2:
        mov     bx, [bx]
        mov     [bx], sp
        NEXT
.9:
        ERROR   ESTKOVFLO
_END:
        call    blkexit
        mov     bx, [_dp]
        mov     sp, [bx]
        add     sp, 2
        pop     word [_file]
        pop     word [buf]
        add     sp, 2
        pop     word [bx]
        cmp     bx, _display+2
        ja      .3
        push    word [_unit1+FBUF]
        call    _v6flush
        call    _pmflush
        call    _psexit
.3:
        pop     word [_dp]
        pop     si
        pop     word [_lino]
        NEXT
_CALL:
        mov     bx, si
        lodsw
        add     bx, ax
        push    word [_lino]
        push    si
        push    word [_dp]
        add     cx, _display
        mov     [_dp], cx
        mov     si, bx
        NEXT
_PUSH:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     ax, sp
        add     ax, cx
.2:
        cmp     sp, ax
        jbe     .3
        push    word 0
        jmp     .2
.3:
        NEXT
_POP:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        add     sp, cx
        NEXT
_SDUP:
        mov     bx, sp
        push    word [bx]
        NEXT
_IF:
        pop     ax
        test    al, al
        jz      _TRA
        add     si, 2
        NEXT
_ASRT:
        pop     ax
        test    al, al
        jz      .9
        NEXT
.9:
        ERROR   EASRT
_TRA:
        add     si, [si]
        NEXT
_LINO:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     [_lino], cx
        mov     bx, [_dp]
        mov     bx, [bx]
        cmp     [bx], sp
        jne     .2
        add     word [_stcnt], 1
        adc     word [_stcnt+2], 0
        sub     word [_stlim], 1
        sbb     word [_stlim+2], 0
        jnz     .1a
        cmp     word [_stlim], 0
        jne     .1a
        ERROR   ESTLIM
.1a:
        NEXT
.2:
        ERROR   ESTKNEMP

        segment _DATA
_stcnt: dd      0
_stlim: dd      500000

        segment _TEXT
_GOTO:
        add     si, [si]
        add     cx, _display
.1:
        mov     bx, [_dp]
        mov     di, [bx]
        cmp     cx, bx
        je      .2
        cmp     bx, _display
        je      .9
        call    blkexit
        mov     ax, [di+2]
        mov     [_file], ax
        mov     ax, [di+4]
        mov     [buf], ax
        mov     ax, [di+8]
        mov     [bx], ax
        mov     ax, [di+10]
        mov     [_dp], ax
        jmp     .1
.2:
        mov     sp, [di]
        NEXT
.9:
        ERROR   EGOTO
;
; blkexit: close the files of the block being left, which keeps every
; register but AX
;
blkexit:
        push    bx
        push    cx
        push    dx
        push    di
        mov     di, [fchain]
.1:
        test    di, di
        jz      .2
        mov     bx, [_dp]
        mov     ax, [bx]
        cmp     [di+FLEV], ax
        ja      .2
        test    word [di+FUNIT], FWRITE
        jz      .4
        push    word [di+FBUF]
        call    _v6flush
        add     sp, 2
.4:
        mov     ax, [di+FUNIT]
        test    ax, ax
        js      .3
        and     ax, 15
        push    ax
        call    _close
        add     sp, 2
        test    ax, ax
        jns     .3
        mov     ax, [di+PFNAME]
        mov     [_file], ax
        ERROR   ECLOSE
.3:
        test    word [di+FUNIT], TEMP
        jz      .5
        push    word [di+PFNAME]
        call    _unlink
        add     sp, 2
        test    ax, ax
        jns     .5
        mov     ax, [di+PFNAME]
        mov     [_file], ax
        ERROR   EREMOVE
.5:
        push    word [di+FNAME]
        call    _pfree
        add     sp, 2
        mov     ax, di
        mov     di, [di+FCHAIN]
        sub     ax, FBUFSZ+14
        push    ax
        call    _pfree
        add     sp, 2
        jmp     .1
.2:
        mov     [fchain], di
        pop     di
        pop     dx
        pop     cx
        pop     bx
        ret
;
; pmflush(): write the profile's counts, pmon.out, if there are any
;
_pmflush:
        cmp     word [_pxpbuf], 0
        je      .1
        push    word 0x1A4              ; 0644
        push    word pmonout
        call    _creat
        add     sp, 4
        test    ax, ax
        js      .9
        push    word [_pxpsize]
        push    word [_pxpbuf]
        push    ax
        call    _write
        add     sp, 6
        test    ax, ax
        js      .9
.1:
        ret
.9:
        push    word pmonout
        call    _perror
        push    word 1
        call    __exit

        segment _DATA
_pxpbuf: dw     0
_pxpsize: dw    0
pmonout: db     "pmon.out", 0

        segment _TEXT
