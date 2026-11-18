; Start-up and console for test programs (NASM, object file).
;
; When main returns, its value is reported on the console as a line
; "@exit NN" (two hex digits), which is what tests/harness/runtest.py waits
; for, and the processor halts.

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
        mov     di, ax
        mov     si, exitmsg
.msg:   mov     al, [cs:si]
        inc     si
        or      al, al
        jz      .code
        call    emit
        jmp     .msg
.code:  mov     ax, di          ; high digit, then low
        shr     al, 4
        call    digit
        mov     ax, di
        call    digit
        mov     al, 13
        call    emit
        mov     al, 10
        call    emit
.hang:  hlt
        jmp     .hang

exitmsg: db     13, 10, "@exit ", 0

digit:  and     al, 0x0F
        add     al, '0'
        cmp     al, '9'
        jbe     emit
        add     al, 'a' - '9' - 1
emit:   push    ax              ; write AL to the console
        call    _putc
        pop     ax
        ret

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
