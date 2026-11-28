; strcpy (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large (../crt/DEFS.inc).
;
; ../../gen/strcpy.c, as ../../pdp/gen has it in assembly: REPNE SCASB
; finds how long the string is, as strlen.s, and REP MOVSB copies it
; with its 0.

%include "DEFS.inc"

S1      equ     2+RA            ; the arguments, from BP: to
S2      equ     S1+PTRSIZE      ; and from

ENTRY _strcpy
        push    bp
        mov     bp, sp
        push    si
        push    di
        SAVEDS
        LESDI   S2
        mov     cx, -1
        xor     al, al
        repne scasb
        not     cx              ; the bytes, and the 0
        LDSSI   S2
        LESDI   S1
        rep movsb
        mov     ax, [bp+S1]
%ifdef FARDATA
        mov     dx, [bp+S1+2]
%endif
        RESTDS
        pop     di
        pop     si
        pop     bp
        RET
