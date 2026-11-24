; Start-up for /boot, and what it asks of the BIOS (NASM, object file).
;
; The boot block (hdboot.s) loads /boot at 0100:0000 and enters at its
; first byte, in real mode, with the BIOS's number for the drive in DL.
; The file is the program's code and data as they are in memory, one
; after the other in a single segment: CS, DS and SS are all that
; segment, and a pointer is an offset in it.  This must be the first
; file linked.
;
; The stack goes at the top of the first 64K of memory.  The kernel is
; loaded above that, and when it starts it has all of /boot's memory to
; give away.

        bits    16
        cpu     286

STACK   equ     0xF000          ; 0100:F000 is 64K

        extern  _main
        extern  _edata:wrt DGROUP, _end:wrt DGROUP
        global  _biosgeom, _biosread, _biosreset, _biosputc, _bioskey
        global  _biosmem, _biosext, _extmove, _inb, _outb, _fcopy, _fclear
        global  _startkernel, _startboot
        global  _swap13, _swapold, _swapdrv, _swapxor, _swapend

        segment _TEXT public align=2 class=CODE
        group   DGROUP _TEXT

..start:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, STACK
        sti
        cld
        mov     di, _edata      ; what the file does not hold starts as
        mov     cx, _end        ;   zero
        sub     cx, di
        xor     al, al
        rep stosb
        xor     dh, dh
        push    dx
        call    _main           ; main(drive), which does not return
.stop:  jmp     .stop

; biosgeom(drive, &heads, &sectors): the drive's shape, as the BIOS
; counts.  Returns 0, or -1 if the BIOS has no such drive.
_biosgeom:
        push    bp
        mov     bp, sp
        push    di
        mov     dl, [bp+4]
        mov     ah, 8
        int     0x13
        mov     ax, -1
        jc      .out
        and     cx, 0x3F
        jz      .out
        mov     bx, [bp+8]
        mov     [bx], cx
        mov     cl, dh
        inc     cx
        mov     bx, [bp+6]
        mov     [bx], cx
        xor     ax, ax
.out:   pop     di
        pop     bp
        ret

; biosread(drive, cylinder, head, sector, count, buffer): read sectors
; of one track.  Returns 0, or the BIOS's number for what went wrong.
_biosread:
        push    bp
        mov     bp, sp
        mov     dl, [bp+4]
        mov     ax, [bp+6]
        mov     ch, al
        shl     ah, 6
        mov     cl, [bp+10]
        or      cl, ah
        mov     dh, [bp+8]
        mov     al, [bp+12]
        mov     bx, [bp+14]
        push    ds
        pop     es
        mov     ah, 2
        int     0x13
        mov     al, ah
        mov     ah, 0
        jc      .out
        xor     ax, ax
.out:   pop     bp
        ret

; biosreset(drive): start the drive over, after an error.
_biosreset:
        push    bp
        mov     bp, sp
        mov     dl, [bp+4]
        xor     ax, ax
        int     0x13
        pop     bp
        ret

; biosputc(c): write a character on the display.
_biosputc:
        push    bp
        mov     bp, sp
        mov     al, [bp+4]
        mov     ah, 0x0E
        mov     bx, 7
        int     0x10
        pop     bp
        ret

; bioskey(): the next character typed at the keyboard, or -1 if none is
; waiting.
_bioskey:
        mov     ah, 1
        int     0x16
        mov     ax, -1
        jz      .out
        xor     ax, ax
        int     0x16
        xor     ah, ah
.out:   ret

; biosmem(): K of memory below 640K.
_biosmem:
        int     0x12
        ret

; biosext(): K of memory above 1M.
_biosext:
        mov     ah, 0x88
        int     0x15
        jnc     .out
        xor     ax, ax
.out:   ret

; extmove(buffer, low, high): have the BIOS copy the 1024 bytes at
; buffer to the physical address high:low, which may be above 1M, where
; nothing else reaches from real mode.  Returns 0, or -1 if it did not.
_extmove:
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        pop     es
        mov     di, mvgdt
        mov     si, di
        mov     cx, 24
        xor     ax, ax
        rep stosw
        mov     word [si+16], 0xFFFF    ; from the buffer
        mov     ax, ds
        mov     dx, ax
        shl     ax, 4
        shr     dx, 12
        add     ax, [bp+4]
        adc     dx, 0
        mov     [si+18], ax
        mov     [si+20], dl
        mov     byte [si+21], 0x93      ; data, to be written
        mov     word [si+24], 0xFFFF    ; to there
        mov     ax, [bp+6]
        mov     [si+26], ax
        mov     al, [bp+8]
        mov     [si+28], al
        mov     byte [si+29], 0x93
        mov     cx, 512                 ; words
        mov     ah, 0x87
        int     0x15
        mov     ax, -1
        jc      .out
        xor     ax, ax
.out:   pop     di
        pop     si
        pop     bp
        ret
mvgdt:  times   48 db 0         ; the descriptors the BIOS moves by

; inb(port), outb(port, value)
_inb:   push    bp
        mov     bp, sp
        mov     dx, [bp+4]
        in      al, dx
        xor     ah, ah
        pop     bp
        ret

_outb:  push    bp
        mov     bp, sp
        mov     dx, [bp+4]
        mov     al, [bp+6]
        out     dx, al
        pop     bp
        ret

; fcopy(to, from, count): copy bytes of this program's to anywhere in
; memory; to is a far pointer.
_fcopy: push    bp
        mov     bp, sp
        push    si
        push    di
        les     di, [bp+4]
        mov     si, [bp+8]
        mov     cx, [bp+10]
        shr     cx, 1
        rep movsw
        adc     cx, cx
        rep movsb
        pop     di
        pop     si
        pop     bp
        ret

; fclear(to, count): clear bytes anywhere in memory.
_fclear:
        push    bp
        mov     bp, sp
        push    di
        les     di, [bp+4]
        mov     cx, [bp+8]
        xor     ax, ax
        shr     cx, 1
        rep stosw
        adc     cx, cx
        rep stosb
        pop     di
        pop     bp
        ret

; startkernel(table, cs, ip, ds): enter the kernel as docs/boot.md says:
; ES:SI the table, DS its data segment, DL the drive.
_startkernel:
        mov     bp, sp
        mov     si, [bp+2]
        mov     dl, [si+10]     ; bi_drive
        push    word [bp+4]
        push    word [bp+6]
        mov     ax, [bp+8]
        push    ds
        pop     es
        mov     ds, ax
        retf

; startboot(): start the sector chain() put at 0:7C00, as the BIOS
; starts a disk's first: DL the first hard disk, 80h.
_startboot:
        cli
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0x7C00
        sti
        mov     dx, 0x80
        push    ax
        mov     bx, 0x7C00
        push    bx
        retf

; The handler of INT 13h that chain() leaves at the top of the memory
; below 640K when it starts a disk other than the first: it swaps the
; BIOS's numbers for the first hard disk, 80h, and that one, swapdrv,
; so that the system started finds itself on the first, as a PC's
; systems expect to.  It runs where it is copied to, with a CS that
; gives it the offsets it has here.  DL is given back as it was, but
; for the two calls that answer in it: 08h, a drive's shape and how
; many there are, and 15h, its type and size.
_swap13:
        cmp     dl, 0x80
        je      .ours
        cmp     dl, [cs:_swapdrv]
        je      .ours
        jmp     far [cs:_swapold]
.ours:  push    bp
        mov     bp, sp
        push    ax              ; [bp-1]: the call
        push    dx              ; [bp-4]: the drive asked for
        xor     dl, [cs:_swapxor]
        pushf
        call    far [cs:_swapold]
        pushf
        cmp     byte [bp-1], 0x08
        je      .keep
        cmp     byte [bp-1], 0x15
        je      .keep
        mov     dl, [bp-4]
.keep:  popf
        mov     sp, bp
        pop     bp
        retf    2
_swapold: dd    0               ; the BIOS's handler
_swapdrv: db    0
_swapxor: db    0               ; 80h ^ swapdrv
_swapend:
