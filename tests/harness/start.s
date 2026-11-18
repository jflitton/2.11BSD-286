; Start-up and console for test programs (NASM, object file).
;
; main's return value ends the run: with QEMU's isa-debug-exit device at
; port 0xF4, writing it there makes QEMU exit.

        bits    16
        cpu     286

        extern  _main
        global  _putc

        segment _TEXT public align=2 class=CODE

..start:
        fninit
        mov     dx, 0x3F8 + 3   ; COM1: 8 bits, no parity, 115200
        mov     al, 0x80
        out     dx, al
        mov     dx, 0x3F8
        mov     al, 1
        out     dx, al
        mov     dx, 0x3F8 + 1
        xor     al, al
        out     dx, al
        mov     dx, 0x3F8 + 3
        mov     al, 3
        out     dx, al
        call    _main
        out     0xF4, al
.hang:  hlt
        jmp     .hang

; putc(c): write one character to COM1, waiting for the transmitter.
_putc:  push    bp
        mov     bp, sp
        mov     dx, 0x3F8 + 5
.wait:  in      al, dx
        test    al, 0x20
        jz      .wait
        mov     dx, 0x3F8
        mov     al, [bp+4]
        out     dx, al
        pop     bp
        ret
