; Start of a small-model test program (NASM), and the few things its C
; cannot do for itself.  The kernel starts a program with the stack
; pointer at the count of arguments; after it are the pointers to them, a
; null, the pointers to the environment and a null.

        bits    16
        cpu     286

; This file is linked first, so the order of segments here is their order
; in the program.  NULL comes first in the data segment: nothing a
; program can point at may be at offset 0 there, where its address would
; be a null pointer.
        segment _TEXT public align=2 class=CODE
        segment _NULL public align=2 class=BEGDATA
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _NULL _DATA _BSS

        segment _NULL
        dw      0, 0

        extern  _main, __exit
        global  _environ, _sigtramp, _getsp, _getcs, _getds, _startbrk

        segment _BSS
_environ:
        resw    1
brk0:   resw    1

        segment _TEXT
..start:
        mov     [brk0], bx      ; the kernel says where the break is
        mov     bx, sp
        mov     ax, [bx]        ; argc
        lea     cx, [bx+2]      ; argv
        mov     dx, ax
        inc     dx
        shl     dx, 1
        add     dx, cx          ; envp, after argv and its null
        mov     [_environ], dx
        push    dx
        push    cx
        push    ax
        call    _main
        push    ax
        call    __exit
.hang:  jmp     .hang

; The kernel continues a process here to take a signal, with the address
; of the handler in DX:AX and on the stack the signal's number, a code,
; the far address of the context to resume and that context.  Call
; handler(sig, code, scp), then resume the context.
_sigtramp:
        mov     bx, sp
        push    word [bx+4]
        push    word [bx+2]
        push    word [bx]
        call    ax
        add     sp, 6
        mov     bx, sp
        mov     cx, [bx+6]
        mov     bx, [bx+4]
        mov     ax, 103         ; sigreturn
        int     0x30
.hang:  jmp     .hang

_startbrk:
        mov     ax, [brk0]
        ret
_getsp: mov     ax, sp
        ret
_getcs: mov     ax, cs
        ret
_getds: mov     ax, ds
        ret

; outb(): an instruction a process may not execute.
        global  _outb
_outb:  out     0x80, al
        ret
