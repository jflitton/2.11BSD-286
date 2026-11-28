; strlen (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large (../crt/DEFS.inc).
;
; ../../gen/strlen.c, as ../../pdp/gen has it in assembly: REPNE SCASB
; finds the 0, counting CX down from -1 for each byte it looks at, the
; 0's too.

%include "DEFS.inc"

S       equ     2+RA            ; the argument, from BP

ENTRY _strlen
        push    bp
        mov     bp, sp
        push    di
        LESDI   S
        mov     cx, -1
        xor     al, al
        repne scasb
        mov     ax, -2          ; -1, less the bytes, less one for the 0
        sub     ax, cx
        pop     di
        pop     bp
        RET
