; strcmp (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large (../crt/DEFS.inc).
;
; ../../gen/strcmp.c, as ../../pdp/gen has it in assembly: what it
; returns is the first bytes that differ, the one less the other, as
; chars, which are signed.

%include "DEFS.inc"

S1      equ     2+RA            ; the arguments, from BP
S2      equ     S1+PTRSIZE

ENTRY _strcmp
        push    bp
        mov     bp, sp
        push    si
        push    di
        SAVEDS
        LDSSI   S1
        LESDI   S2
.next:  lodsb                   ; *s1++
        scasb                   ; against *s2++
        jne     .differ
        test    al, al
        jnz     .next
        xor     ax, ax          ; the same to the end
        jmp     .out
.differ:
        cbw
        mov     cx, ax
        mov     al, [es:di-1]
        cbw
        sub     cx, ax
        mov     ax, cx
.out:   RESTDS
        pop     di
        pop     si
        pop     bp
        RET
