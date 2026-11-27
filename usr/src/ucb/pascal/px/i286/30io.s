;
; IO operations
;
_GET:
        call    get
        call    iosync
        NEXT
_PUT:
        call    put
        NEXT
_MESSAGE:
        mov     ax, _unit2
        call    unit
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
        NEXT
_FNIL:
        mov     bx, sp
        mov     ax, [bx]
        push    word [_file]
        push    word [buf]
        call    unit
        call    iosync
        pop     word [buf]
        pop     word [_file]
        NEXT
_BUFF:
        mov     [bufopt], cx
        NEXT
_EOF:
        pop     ax
        push    word [_file]
        push    word [buf]
        push    word EOF
        jmp     eofeoln
_EOLN:
        pop     ax
        push    word [_file]
        push    word [buf]
        push    word EOF+EOLN
eofeoln:
        call    unit
        cmp     word [buf], _unit0
        jne     .1
        cmp     word [bufopt], 1
        jne     .1
        push    word u1buf
        call    _v6flush
        add     sp, 2
.1:
        pop     dx                      ; what is asked
        push    word 0
        mov     bx, [buf]
        test    word [bx+FUNIT], EOF
        jnz     .2
        call    iosync
        test    [bx+FUNIT], dx
        jz      .3
.2:
        mov     di, sp
        mov     word [di], 1
.3:
        pop     ax
        pop     word [buf]
        pop     word [_file]
        push    ax
        NEXT
_RESET:
        jnz     .1                      ; branch if name given
        mov     bx, sp
        mov     bx, [bx]
        cmp     word [bx], _unit0
        jne     .1
        cmp     word [_unit0+FNAME], 0
        jne     .1
        push    word 0
        push    word 0
        push    word 0
        push    word 0
        call    _lseek
        add     sp, 8
        test    dx, dx
        js      .9
        add     sp, 2
        add     si, 2
        and     word [_unit0+FUNIT], 0xFFFF ^ (EOF+EOLN)
        or      word [_unit0+FUNIT], SYNC
        NEXT
.9:
        mov     word [_file], stdin
        ERROR   ESEEK
.1:
        call    getname
        mov     di, ax
        mov     ax, [di+PFNAME]
        mov     [_file], ax
        push    word 0
        push    word [di+FNAME]
        call    _open
        add     sp, 4
        test    ax, ax
        js      eopen
        mov     bx, [di+FBUF]
        mov     [bx], ax
        or      ax, SYNC|FREAD
        or      [di+FUNIT], ax
        NEXT
ecreat:
        ERROR   ECREATE
eopen:
        ERROR   EOPEN
_REWRITE:
        call    getname
        mov     di, ax
        mov     ax, [di+PFNAME]
        mov     [_file], ax
        push    word 0x1A4              ; 0644
        push    word [di+FNAME]
        call    _creat
        add     sp, 4
        test    ax, ax
        js      ecreat
        push    ax
        call    _close
        add     sp, 2
        push    word 1
        push    word [di+FNAME]
        call    _open
        add     sp, 4
        test    ax, ax
        js      eopen
        mov     bx, [di+FBUF]
        mov     [bx], ax
        or      ax, EOF|FWRITE
        or      [di+FUNIT], ax
        NEXT
;
; remove: the file the string on the stack names, blanks after it
; taken off.  The PDP-11's took the string off the stack first and
; pushed unlink's argument over it; here it is taken off after.
;
_REMOVE:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     di, sp                  ; the name
        mov     bp, di
        add     bp, cx                  ; and its end
        mov     al, [bp]
        push    ax                      ; what is after it, put back
        mov     byte [bp], 0
        mov     bx, bp
.2:
        cmp     byte [bx-1], ' '
        jne     .3
        dec     bx
        mov     byte [bx], 0
        loop    .2
.3:
        mov     [_file], di
        push    di
        call    _unlink
        add     sp, 2
        pop     dx
        mov     [bp], dl
        test    ax, ax
        js      .9
        mov     ax, bp
        sub     ax, di
        inc     ax
        and     al, 0xFE
        add     ax, di
        mov     sp, ax
        NEXT
.9:
        ERROR   EREMOVE
_UNITINP:
        cmp     word [bufopt], 2
        jge     .1                      ; bufopt >= 2, don't flush on read
        cmp     word [u1cnt], 512
        je      .1
        push    word u1buf
        call    _v6flush
        add     sp, 2
.1:
        mov     word [buf], _unit0
        mov     word [_file], stdin
        NEXT
_UNITOUT:
        mov     word [buf], _unit1
        mov     word [_file], stdout
        NEXT
_UNIT:
        pop     ax
        cmp     ax, _unit0
        je      _UNITINP
        call    unit
        NEXT
_READ8:
        sub     sp, 32
        mov     ax, sp
        push    ax
        push    word 32
        push    word 1
        call    _preadn
        jc      .1
        add     sp, 6
        mov     ax, sp
        push    ax
        call    _atof
        add     sp, 26
        mov     bx, sp
        fstp    qword [bx]
        fwait
        NEXT
.1:
        add     sp, 38
        push    word 0
        push    word 0
        push    word 0
        push    word 0
        NEXT
_READ4:
        sub     sp, 20
        mov     ax, sp
        push    ax
        push    word 20
        push    word 0
        call    _preadn
        jc      .1
        add     sp, 6
        mov     ax, sp
        push    ax
        call    _atol
        add     sp, 22
        push    dx
        push    ax
        NEXT
.1:
        add     sp, 26
        push    word 0
        push    word 0
        NEXT
_READC:
        call    iosync
        mov     bx, [buf]
        mov     al, [bx]
        xor     ah, ah
        push    ax
        call    get
        NEXT
_READLN:
        call    iosync
        mov     bx, [buf]
        test    word [bx+FUNIT], EOLN+EOF
        jnz     .1
        call    get
        jmp     _READLN
.1:
        call    get
        NEXT
_PAGE:
        push    word 12
        jmp     writc1
_WRITC:
        jz      writc1
        mov     ax, O_WRITC
        jmp     pwriteit
writc1:
        pop     ax
        mov     bx, [buf]
        mov     [bx], al
        call    put
        cmp     word [bufopt], 0
        je      wflush
        NEXT
_WRITLN:
        mov     ax, O_WRITLN
        push    word 10
        mov     di, 1
        cmp     word [buf], _unit1
        jne     pwrite1
        cmp     word [bufopt], 1
        jg      pwrite1
        xor     di, di
        jmp     pwrite1
_WRITG:
        mov     ax, O_WRITG
        push    word [si]
        add     si, 2
        jmp     pwriteit
;
; The PDP-11's gave pwrite 0267 for WRHEX2, which is no operation of
; pwrite's: O_WRHEX2 is 0276.
;
_WRHEX2:
        mov     ax, O_WRHEX2
        jmp     pwriteit
_WRHEX4:
        mov     ax, O_WRHEX2+1
        jmp     pwriteit
_WROCT2:
        mov     ax, O_WROCT2
        jmp     pwriteit
_WROCT4:
        mov     ax, O_WROCT2+1
        jmp     pwriteit
_WRIT2:
        mov     ax, O_WRIT2
        jmp     pwriteit
_WRIT4:
        mov     ax, O_WRIT4
        jmp     pwriteit
_WRIT8:
        mov     ax, O_WRIT8
        jmp     pwriteit
_WRITB:
        mov     ax, O_WRITB
        jmp     pwriteit
_WRIT82:
        mov     ax, O_WRIT82
;
; pwriteit: pwrite(AX, the subop, what is on the stack), which gives
; back where the stack is after what it wrote.  The output is flushed
; after if DI, bufopt, is 0.
;
pwriteit:
        mov     di, [bufopt]
pwrite1:
        push    cx
        push    ax
        call    _pwrite
        mov     sp, ax
        test    di, di
        jnz     wdone
wflush:
        cmp     word [buf], _unit1
        jne     wdone
        mov     bx, [buf]
        push    word [bx+FBUF]
        call    _v6flush
        add     sp, 2
wdone:
        NEXT
_DEFNAME:
        call    getname
        mov     bx, ax
        or      word [bx+FUNIT], 0x8000
        NEXT
_FLUSH:
        pop     ax
        call    unit
        mov     bx, [buf]
        test    word [bx+FUNIT], FWRITE
        jz      .1
        push    word [bx+FBUF]
        call    _v6flush
        add     sp, 2
.1:
        NEXT
