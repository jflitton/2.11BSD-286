;
; getname
;
; takes the width of a string in CX
; returns a pointer to a file structure in AX
;
; there should be a string on the stack
; of length the contents of CX on top of
; a pointer to the file variable
;
; a new file structure is allocated if needed
; temporary names are generated, and given
; names are blank trimmed
;
; if a new file buffer is allocated, the address
; is stored throught the file variable pointer
;
; The string is BP's, which C keeps, and the window is DI's; CX, which
; C does not keep, is kept in glen across what is called.
;
getname:
        pop     word [reta]
        mov     bp, sp                  ; the string
        mov     bx, cx
        inc     bx
        and     bx, 0xFFFE
        add     bx, sp
        mov     ax, [bx]                ; the file variable
        add     bx, 2
        mov     [newsp], bx
        mov     bx, ax
        cmp     word [bx], 0
        jne     gotone
;
; dont have a file struct
; so alloc a new one
;
        push    ax
        mov     ax, [si]
        test    ax, ax
        jnz     .1
        mov     ax, 1
.1:
        add     ax, FBUFSZ+14
        mov     [glen], cx
        push    ax
        call    _alloc
        add     sp, 2
        mov     cx, [glen]
        mov     bx, ax
        mov     [bx+FBUFSZ], ax
        add     ax, FBUFSZ+14
        mov     bx, ax                  ; the window
        lodsw
        mov     [bx+FSIZE], ax
        test    ax, ax
        jnz     .2
        or      word [bx+FUNIT], FTEXT
        mov     word [bx+FSIZE], 1
.2:
        mov     di, sp
        mov     dx, [di]
        mov     [bx+FLEV], dx
        push    word fchain-FCHAIN      ; get head of file chain
        mov     di, [fchain]            ; contents of head of file chain
.3:
        test    di, di                  ; end of the chain?
        jnz     .4
        pop     di                      ; get last chain entry
        mov     [di+FCHAIN], bx         ; and stuff the current file in
        mov     word [bx+FCHAIN], 0     ; last entry
        jmp     .6
.4:
        cmp     [di+FLEV], dx           ; are we past it yet?
        je      .6
        ja      .5
        pop     ax                      ; advance the chain
        push    di
        mov     di, [di+FCHAIN]
        jmp     .3
.5:
        mov     [bx+FCHAIN], di
        pop     di                      ; address of last entry
        mov     [di+FCHAIN], bx         ; stuff in the current entry
.6:
        pop     di
        mov     [di], bx
        mov     ax, bx
        jmp     gnname
;
; have a previous buffer
; close the associated file
;
gotone:
        add     si, 2
        mov     di, [bx]
        mov     ax, [di+FUNIT]
        test    ax, ax
        jg      .3                      ; from name from prog hedr
        and     word [di+FUNIT], 0x7FFF
        jmp     .1
.3:
        mov     [glen], cx
        test    word [di+FUNIT], FWRITE
        jz      .6
        push    ax
        push    word [di+FBUF]
        call    _v6flush
        add     sp, 2
        pop     ax
.6:
        and     ax, 15
        push    ax
        call    _close
        add     sp, 2
        mov     cx, [glen]
        test    ax, ax
        js      .9
        test    word [di+FUNIT], TEMP
        jz      .1
        test    cx, cx
        jz      .1
        call    cleanup
        mov     cx, [glen]
.1:
        and     word [di+FUNIT], TEMP+FTEXT
        mov     bx, [di+FBUF]
        mov     word [bx], 0            ; clear the getc/putc buffer
        mov     word [bx+2], 0
        mov     word [bx+4], 0
        mov     ax, di
        jmp     gnname
.9:
        ERROR   ECLOSE
;
; get the filename to the
; buffer (not necess. easy)
;
gnname:
        mov     bx, ax
        test    cx, cx
        jnz     .2
;
; no name given
;
        cmp     word [bx+FNAME], 0
        je      .1
;
; no name given and had
; a prev name so use it
; again
;
        mov     sp, [newsp]
        jmp     [reta]
;
; no name given and no
; prev name so generate
; a new one of the form
; "tmp.x"
;
.1:
        or      word [bx+FUNIT], TEMP
        inc     word [nextu]
        mov     al, [nextu]
        mov     [tnam+4], al
        mov     bp, tnam
        push    word 6
        jmp     alname
;
; name given
; strip trailing blanks
;
.2:
        and     word [bx+FUNIT], 0xFFFF ^ TEMP
        add     bp, cx
        mov     byte [bp], 0
.3:
        cmp     byte [bp-1], ' '
        jne     .4
        dec     bp
        mov     byte [bp], 0
        dec     cx
        jnz     .3
.4:
        sub     bp, cx
        inc     cx
        push    cx
;
; save the new name
; in dynamic space
;
alname:
        mov     di, bx
        call    _alloc
        mov     bx, ax
.1:
        mov     dl, [bp]
        mov     [bx], dl
        inc     bp
        inc     bx
        test    dl, dl
        jnz     .1
        mov     bp, ax
;
; free previous file name
; (if any)
;
        mov     ax, [di+FNAME]
        test    ax, ax
        jz      .2
        push    ax
        call    _pfree
        add     sp, 2
;
; put the new name
; into the structure
;
.2:
        mov     [di+FNAME], bp
        mov     [di+PFNAME], bp
        mov     ax, di
        mov     sp, [newsp]
        jmp     [reta]

        segment _BSS
reta:   resw    1
newsp:  resw    1
glen:   resw    1

        segment _TEXT
;
; cleanup: the temporary file of the window DI is removed
;
cleanup:
        push    word [di+PFNAME]
        call    _unlink
        add     sp, 2
        test    ax, ax
        jns     .1
        mov     ax, [di+PFNAME]
        mov     [_file], ax
        ERROR   EREMOVE
.1:
        push    word [di+PFNAME]
        call    _pfree
        add     sp, 2
        ret
