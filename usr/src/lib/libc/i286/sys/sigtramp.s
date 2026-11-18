; The signal trampoline, small model (NASM).  sigaction() names it to the
; kernel, which continues a process here to take a signal
; (docs/processes.md), with the address of the handler in DX:AX and on
; the stack the signal's number, a code, the far address of the context
; to resume, and that context.  Call handler(sig, code, scp), then resume
; the context.

        bits    16
        cpu     286

SYS_sigreturn equ 103

        segment _TEXT public align=2 class=CODE
        global  __sigtramp

__sigtramp:
        push    ss              ; the handler's data, whatever the process
        pop     ds              ;   was doing with DS
        push    ss
        pop     es
        mov     bx, sp
        push    word [bx+4]     ; scp, near
        push    word [bx+2]     ; code
        push    word [bx]       ; sig
        call    ax
        add     sp, 6
        mov     bx, sp
        mov     cx, [bx+6]
        mov     bx, [bx+4]
        mov     ax, SYS_sigreturn
        int     0x30
        hlt                     ; the context was refused: nothing to go
                                ;   back to, so fault
