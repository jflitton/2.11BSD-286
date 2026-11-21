; alloca (NASM): small model, with -DFAR medium, and with -DFARDATA as
; well large, where what it returns is far and the stack is reached
; through SS.  For what the system's own compiler compiles
; (../../../ccom/i286); the cross compiler is given alloca in line
; (<stdlib.h>).
;
; char *alloca(n): n bytes on the caller's stack, which are gone when it
; returns: it takes its stack pointer back from its frame pointer.
;
; The compiler's functions save SI and DI, those that use them, below
; their variables, and take them back from the top of the stack before
; they return.  So the two words that were there are put on top again,
; under the new space; if they were not saved registers nothing looks at
; the copies.  The caller then takes its argument off, for which there
; is a word.
;
; As on the PDP-11, alloca called for an argument, when others of the
; same call have been pushed, leaves those above the new space, where
; the function called will not find them.

%include "DEFS.inc"

ENTRY _alloca
        pop     dx              ; the return address
%ifdef FAR
        pop     cx
%endif
        pop     ax              ; n
        mov     bx, sp          ; the caller's stack, as it will be again
        inc     ax
        and     al, 0xFE
        sub     sp, ax
        mov     ax, sp
%ifdef FARDATA
        push    word [ss:bx+2]  ; SI, if it was saved
        push    word [ss:bx]    ; DI
%else
        push    word [bx+2]     ; SI, if it was saved
        push    word [bx]       ; DI
%endif
        push    ax              ; in place of the argument
%ifdef FAR
        push    cx
%endif
        push    dx
%ifdef FARDATA
        mov     dx, ss
%endif
        RET
