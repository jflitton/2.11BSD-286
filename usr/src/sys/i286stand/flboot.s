; Diskette boot block (NASM, flat binary).
;
; The first sector of a diskette, which is the first sector of the file
; system that is all of it (../i286at/fl.c).  The BIOS loads it.  It
; finds the file /boot in that file system, loads it at 0100:0000 and
; starts it there with
;
;       DL      the BIOS's number for the drive
;       SS:SP   a stack at the top of the first 64K
;
; and with interrupts on.  docs/boot.md has the rest.  It is hdboot.s
; but for what a diskette has not, a partition table and cylinders past
; the BIOS's reach, and for how it learns the diskette's shape.
;
; It moves itself to 0600 first, to be out of the way of what it loads.
;
; The file system's layout is sys/h/fs.h, inode.h and dir.h.  /boot must
; be in the root directory and be no more than 56K.

        bits    16
        cpu     286

BASE    equ     0x0600          ; where this runs
CODE    equ     446             ; bytes of it
LOAD    equ     0x0100          ; where /boot goes, as a segment

; What is kept above the code.  A file's first four block addresses sit
; just under the block that has the rest of them, so that the address of
; block N of the file is at ADDR + 4 * N wherever it came from.
nsect   equ     BASE + CODE     ; sectors on a track
nhead   equ     nsect + 2       ; heads
drive   equ     nhead + 2       ; the BIOS's number for the drive
BUF     equ     0x07EC          ; a block: of inodes, or of a directory
SIZE    equ     BUF + 1024      ; the file's size in bytes
ADDR    equ     SIZE + 4        ; its first four block addresses,
IND     equ     ADDR + 16       ;   and its block of 256 more

ROOTINO equ     2
INOPB   equ     16              ; inodes in a block

        org     BASE

        cli
        xor     ax, ax
        mov     ss, ax
        mov     sp, ax
        mov     ds, ax
        mov     es, ax
        sti
        cld
        mov     si, 0x7C00
        mov     di, BASE
        mov     cx, CODE / 2
        rep movsw
        jmp     0:go

go:     mov     [drive], dl
        mov     word [nhead], 2
; How many sectors a track has: the BIOS knows what the drive can hold,
; and not what this diskette does.  It is the most that the first track
; has one of, counting from 1.
        mov     si, sizes
.size:  lodsb
        cbw
        mov     [nsect], ax
        mov     di, 3           ; tries
.try:   mov     cx, [nsect]
        xor     dh, dh
        mov     dl, [drive]
        mov     bx, BUF
        mov     ax, 0x0201
        int     0x13
        jnc     .sized
        xor     ax, ax          ; start the drive over
        int     0x13
        dec     di
        jnz     .try
        cmp     byte [si], 0
        jne     .size
        jmp     ioerr
.sized:
; Look through the root directory for "boot".
        mov     ax, ROOTINO
        call    iget
        xor     cx, cx
.dirblk:
        mov     bx, BUF
        call    fblk
        jnc     nofile
        mov     si, BUF
.entry: cmp     word [si], 0    ; an entry not in use
        je      .next
        cmp     word [si+4], 4  ; the length of its name
        jne     .next
        cmp     word [si+6], 'bo'
        jne     .next
        cmp     word [si+8], 'ot'
        je      .found
.next:  mov     ax, [si+2]      ; the length of the entry
        or      ax, ax
        jz      .dirblk         ; the rest of the block was never used
        add     si, ax
        cmp     si, BUF + 1024
        jb      .entry
        jmp     .dirblk

.found: mov     ax, [si]
        call    iget
        xor     cx, cx
        mov     bx, LOAD << 4
.load:  call    fblk
        jc      .load
        mov     dl, [drive]
        jmp     LOAD:0

nofile: mov     si, nomsg
        jmp     short say
ioerr:  mov     si, iomsg
say:    lodsb
        or      al, al
.stop:  jz      .stop
        mov     ah, 0x0E
        xor     bx, bx
        int     0x10
        jmp     say

; Make inode AX the file: its size, its first four block addresses, and
; its indirect block.  A file without one has the file system's first
; block read in its place, which does no harm: fblk stops at the file's
; size.
iget:   add     ax, 2 * INOPB - 1       ; inode 1 is the first of block 2
        mov     si, ax
        shr     ax, 4
        cwd
        mov     bx, BUF
        call    rdblk
        and     si, INOPB - 1
        shl     si, 6
        add     si, BUF + 8     ; the size, then seven addresses
        mov     di, SIZE
        mov     cx, 16
        rep movsw
        mov     ax, [IND]       ; the fifth is the indirect block's
        mov     dx, [IND+2]
        mov     bx, IND
        jmp     short rdblk

; Read block CX of the file to ES:BX and step both, with the carry set;
; or at the file's end do nothing, with it clear.
fblk:   mov     ax, cx
        shl     ax, 10
        cmp     ax, [SIZE]
        jae     .end
        mov     si, cx
        shl     si, 2
        mov     ax, [ADDR+si]
        mov     dx, [ADDR+si+2]
        call    rdblk
        inc     cx
        stc
.end:   ret

; Read block DX:AX of the file system to ES:BX and step BX: two sectors.
rdblk:  shl     ax, 1
        rcl     dx, 1
        call    rdsect
        add     ax, 1
        adc     dx, 0

; Read sector DX:AX of the disk to ES:BX and step BX.
rdsect: pusha
        xchg    ax, cx          ; divide the top half first, so that
        xchg    ax, dx          ;   neither quotient can be too big
        xor     dx, dx
        div     word [nsect]
        xchg    ax, cx
        div     word [nsect]    ; cx:ax = the track, dx = the sector in it
        inc     dx
        push    dx
        mov     dx, cx
        div     word [nhead]    ; ax = the cylinder, dx = the head
        pop     cx
        mov     ch, al
        shl     ah, 6
        or      cl, ah
        mov     dh, dl
        mov     dl, [drive]
        mov     di, 3           ; tries
.try:   mov     ax, 0x0201
        int     0x13
        jnc     .ok
        xor     ax, ax          ; start the drive over
        int     0x13
        dec     di
        jnz     .try
        jmp     ioerr
.ok:    popa
        add     bh, 2
        ret

iomsg:  db      "boot: cannot read the diskette", 0
nomsg:  db      "boot: no /boot", 0

sizes:  db      18, 15, 9, 0    ; sectors on a track: 1.44 MB, 1.2 MB, 720K

        times   CODE - ($ - $$) db 0
        times   64 db 0         ; where a hard disk has a partition table
        dw      0xAA55
