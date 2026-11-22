; The two far functions that support.c's fr_func gives t_far.c, for where
; support.c is compiled by the system's own compiler and is of the small
; model: that compiler makes a function far only in a model in which all
; of them are, and what calls these calls them far.

        bits    16
        cpu     286

        segment _TEXT public align=2 class=CODE

        global  _fr_add, _fr_mul

; fr_add(a, b)
_fr_add:
        push    bp
        mov     bp, sp
        mov     ax, word [bp+6]
        add     ax, word [bp+8]
        pop     bp
        retf

; fr_mul(a, b)
_fr_mul:
        push    bp
        mov     bp, sp
        mov     ax, word [bp+6]
        imul    word [bp+8]
        pop     bp
        retf
