; The signal trampoline (NASM): small model, with -DFAR medium, or with
; -DFAR and -DFARDATA large.
; sigaction() names it to the kernel, which continues a process here to
; take a signal (docs/processes.md), with the address of the handler in
; DX:AX and on the stack the signal's number, a code, the far address of
; the context to resume, and that context.  Call handler(sig, code, scp),
; then resume the context.

%include "DEFS.inc"

SYS_sigreturn equ 103

        global  __sigtramp

__sigtramp:
        push    ss              ; the handler's data, whatever the process
        pop     ds              ;   was doing with DS
        push    ss
        pop     es
%ifdef FAR
        push    dx              ; the handler, where a far call can find
        push    ax              ;   it, below the arguments
        mov     bx, sp
%ifdef FARDATA
        push    word [bx+10]    ; scp, far
        push    word [bx+8]
        push    word [bx+6]     ; code
        push    word [bx+4]     ; sig
        call    far [bx]
        add     sp, 12
        push    ss              ; the handler left DS as it liked
        pop     ds
%else
        push    word [bx+8]     ; scp, near
        push    word [bx+6]     ; code
        push    word [bx+4]     ; sig
        call    far [bx]
        add     sp, 10
%endif
%else
        mov     bx, sp
        push    word [bx+4]     ; scp, near
        push    word [bx+2]     ; code
        push    word [bx]       ; sig
        call    ax
        add     sp, 6
%endif
        mov     bx, sp
        mov     cx, [bx+6]
        mov     bx, [bx+4]
        mov     ax, SYS_sigreturn
        int     0x30
        hlt                     ; the context was refused: nothing to go
                                ;   back to, so fault
