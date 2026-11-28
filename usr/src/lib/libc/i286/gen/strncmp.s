; strncmp (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large (../crt/DEFS.inc).
;
; ../../gen/strncmp.c, as ../../pdp/gen has it in assembly: strcmp.s
; that stops after n bytes, and n of 0 or less is no bytes.

%include "DEFS.inc"

S1      equ     2+RA            ; the arguments, from BP
S2      equ     S1+PTRSIZE
N       equ     S2+PTRSIZE

ENTRY _strncmp
        push    bp
        mov     bp, sp
        push    si
        push    di
        SAVEDS
        LDSSI   S1
        LESDI   S2
        xor     ax, ax
        mov     cx, [bp+N]
        test    cx, cx
        jle     .out
.next:  lodsb                   ; *s1++
        scasb                   ; against *s2++
        jne     .differ
        test    al, al
        loopnz  .next           ; on to n bytes, or the 0
        xor     ax, ax          ; the same that far
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
