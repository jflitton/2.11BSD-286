; index (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large (../crt/DEFS.inc).
;
; ../../gen/index.c, as ../../pdp/gen has it in assembly: where the
; first c is in the string, its 0 included, or NULL.

%include "DEFS.inc"

S       equ     2+RA            ; the arguments, from BP
C       equ     S+PTRSIZE

ENTRY _index
        push    bp
        mov     bp, sp
        push    si
        SAVEDS
        LDSSI   S
        mov     ah, [bp+C]
.next:  lodsb
        cmp     al, ah
        je      .found
        test    al, al
        jnz     .next
        xor     ax, ax          ; NULL
%ifdef FARDATA
        cwd
%endif
        jmp     .out
.found: lea     ax, [si-1]
%ifdef FARDATA
        mov     dx, ds
%endif
.out:   RESTDS
        pop     si
        pop     bp
        RET
