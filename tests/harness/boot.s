; Boot sector for the test harness (NASM, flat binary).
;
; tests/harness/runtest.py puts a small-model program's code segment in the
; sectors after this one on a 1.44 MB diskette image, its data segment
; after that, and the two sector counts in the words at offset 3 of this
; sector.  This loads them at 1000:0 and 2000:0 and runs the program in
; real mode with DS = SS = 2000.  The routines under test behave the same
; in real and protected mode.

        bits    16
        cpu     286
        org     0x7C00

        jmp     short start
        nop
ncode:  dw      128             ; sectors of code  (offset 3)
ndata:  dw      128             ; sectors of data  (offset 5)

start:  cli
        xor     ax, ax
        mov     ss, ax
        mov     sp, 0x7C00
        mov     ds, ax
        sti
        mov     [drive], dl

        mov     ax, 0x2000      ; clear the data segment, so that bss is zero
        mov     es, ax
        xor     di, di
        xor     ax, ax
        mov     cx, 0x8000
        cld
        rep stosw

        mov     si, 1           ; logical sector
        mov     ax, 0x1000
        mov     bp, [ncode]
        call    load
        mov     ax, 0x2000
        mov     bp, [ndata]
        call    load

        cli
        mov     ax, 0x2000
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xFFFE
        sti
        jmp     0x1000:0

; Read BP sectors, starting at logical sector SI, to AX:0.  Advances SI.
load:   mov     es, ax
        xor     bx, bx          ; es:bx = where the next sector goes
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
        jmp     $               ; give up
.ok:    add     bx, 512
        inc     si
        dec     bp
        jmp     .next
.done:  ret

drive:  db      0
        times   510 - ($ - $$) db 0
        dw      0xAA55
