; Diskette boot sector (NASM, flat binary).
;
; tools/mkfd.py puts an executable's segments in the sectors after this
; one, each starting on a sector, and fills in the table below.  This
; clears and loads each segment where the table says and starts the
; program, in real mode:
;
;       CS:IP   the program's entry point
;       DS      where the program's data segment was loaded
;       ES:SI   the table below (struct bootinfo, ../i286/boot.h)
;       DL      the BIOS's number for the drive booted from
;       SS:SP   a small stack below this sector; interrupts are on
;
; The drive's motor is left running.  A program that takes over the clock
; interrupt, from which the BIOS would have stopped it, must stop it.
;
; The same sector starts the kernel and the real-mode test programs of
; tests/harness.  docs/boot.md describes the table.

        bits    16
        cpu     286
        org     0x7C00

MAXSEG  equ     16

        jmp     near start      ; three bytes

info:                           ; offset 3
        dw      0x0286          ; magic
nseg:   dw      0               ; number of segments
eseg:   dw      0               ; entry point: segment, counting from 1
eoff:   dw      0               ;              offset
dseg:   dw      0               ; the data segment, counting from 1
drive:  dw      0               ; drive booted from; set here
        dw      0               ; a RAM disk image for the kernel to load:
        dw      0               ;   its first sector, and how many
segs:   times MAXSEG * 8 db 0   ; for each segment:
                                ;   where it goes, as a real-mode segment
                                ;   sectors of it on the diskette
                                ;   its size in memory, less one
                                ;   1 code, 2 data

start:  cli
        xor     ax, ax
        mov     ss, ax
        mov     sp, 0x7C00
        mov     ds, ax
        sti
        cld
        mov     [drive], dl

        mov     si, 1           ; the next sector to read
        mov     bx, segs
        mov     cx, [nseg]
.each:  push    cx
        push    bx
        mov     es, [bx]
        mov     cx, [bx+4]      ; clear it, so that bss starts as zero
        shr     cx, 1
        inc     cx
        xor     di, di
        xor     ax, ax
        rep stosw
        mov     bp, [bx+2]
        call    load
        pop     bx
        pop     cx
        add     bx, 8
        loop    .each

        mov     bx, [eseg]
        dec     bx
        shl     bx, 3
        push    word [segs+bx]
        push    word [eoff]
        mov     bx, [dseg]
        dec     bx
        shl     bx, 3
        mov     ax, [segs+bx]
        mov     dl, [drive]
        mov     si, info
        push    ds
        pop     es
        mov     ds, ax
        retf

; Read BP sectors, starting at logical sector SI, to ES:0.  Advances SI.
; One sector at a time: mkfd.py loads segments at multiples of 512 bytes,
; so that no transfer crosses a 64K boundary, which the diskette
; controller's DMA cannot do.
load:   xor     bx, bx          ; es:bx = where the next sector goes
.next:  or      bp, bp
        jz      .done
        mov     ax, si
        mov     cl, 36          ; sectors per cylinder (18 * 2 heads)
        div     cl              ; al = cylinder, ah = sector within it
        mov     ch, al
        mov     al, ah
        xor     ah, ah
        mov     cl, 18
        div     cl              ; al = head, ah = sector
        mov     dh, al
        mov     cl, ah
        inc     cl
        mov     dl, [drive]
        mov     di, 3           ; tries
.retry: mov     ax, 0x0201
        int     0x13
        jnc     .ok
        xor     ax, ax
        int     0x13
        dec     di
        jnz     .retry
        mov     si, failed
.msg:   lodsb
        or      al, al
        jz      .stop
        mov     ah, 0x0E
        xor     bx, bx
        int     0x10
        jmp     .msg
.stop:  jmp     .stop
.ok:    add     bx, 512
        inc     si
        dec     bp
        jmp     .next
.done:  ret

failed: db      "boot: read error", 0

        times   510 - ($ - $$) db 0
        dw      0xAA55
