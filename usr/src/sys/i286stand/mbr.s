; The first sector of a hard disk that is shared between systems (NASM,
; flat binary).
;
; The BIOS loads it and starts it.  It loads the first sector of the part
; of the disk that the partition table marks as the one to boot, and
; starts that as the BIOS would have:
;
;       DL      the BIOS's number for the drive
;       DS:SI   the part's entry in the table
;
; That is what every PC system expects of this sector, so it will start
; another system's part as well as this one's (hdboot.s), and another's
; program here will start this one's.  The table itself is not in this
; file: tools/mkdisk.py puts this code before the table it writes, and
; anything else that installs it must leave bytes 446 to 509 alone.

        bits    16
        cpu     286

BASE    equ     0x0600          ; where this runs
TABLE   equ     BASE + 446

        org     BASE

        cli
        xor     ax, ax
        mov     ss, ax
        mov     sp, 0x7C00
        mov     ds, ax
        mov     es, ax
        sti
        cld
        mov     si, 0x7C00
        mov     di, BASE
        mov     cx, 256
        rep movsw
        jmp     0:go

go:     mov     si, TABLE
        mov     cx, 4
.part:  cmp     byte [si], 0x80
        je      .boot
        add     si, 16
        loop    .part
        mov     si, nomsg
        jmp     short say

.boot:  mov     di, 3           ; tries
.try:   mov     dh, [si+1]      ; its first sector: head,
        mov     cx, [si+2]      ;   sector and cylinder
        mov     bx, 0x7C00
        mov     ax, 0x0201
        int     0x13
        jnc     .ok
        xor     ax, ax          ; start the drive over
        int     0x13
        dec     di
        jnz     .try
        mov     si, iomsg
        jmp     short say
.ok:    cmp     word [0x7DFE], 0xAA55
        jne     .none
        jmp     0:0x7C00
.none:  mov     si, bootmsg

say:    lodsb
        or      al, al
.stop:  jz      .stop
        mov     ah, 0x0E
        xor     bx, bx
        int     0x10
        jmp     say

nomsg:  db      "No part of this disk is marked to boot", 0
iomsg:  db      "Cannot read the disk", 0
bootmsg: db     "Nothing to boot in the part marked", 0

        times   446 - ($ - $$) db 0
        times   64 db 0         ; the table
        dw      0xAA55
