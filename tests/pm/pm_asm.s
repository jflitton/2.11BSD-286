; Assembly for tests/pm: reading the processor's state, and instructions
; that fault on purpose.
;
; A routine that means to fault puts the address to go on from in _resume
; first.  The test's trap() sends the processor there.

%include "mch_defs.inc"

        extern  _resume:wrt DGROUP
        TEXT

; getcs(): the code segment of the caller.
ENTRY getcs
        mov     bx, sp
        mov     ax, [bx+2]
        retf
ENTRY getds
        mov     ax, ds
        retf
ENTRY getss
        mov     ax, ss
        retf
ENTRY getsp
        mov     ax, sp
        retf
ENTRY getmsw
        smsw    ax
        retf
ENTRY gettr
        str     ax
        retf
ENTRY getflags
        pushf
        pop     ax
        retf

; getgdtr(p), getidtr(p): store the 6 bytes of the table's limit and
; address at p.
ENTRY getgdtr
        mov     bx, sp
        mov     bx, [bx+4]
        sgdt    [bx]
        retf
ENTRY getidtr
        mov     bx, sp
        mov     bx, [bx+4]
        sidt    [bx]
        retf

; seglimit(sel): the limit of a segment, or -1 if the selector is no good.
; (No segment in these tests has a limit of 0xFFFF.)
ENTRY seglimit
        mov     bx, sp
        mov     ax, -1
        lsl     ax, [bx+4]
        retf

; segaccess(sel): the access byte of a segment, or -1.
ENTRY segaccess
        mov     bx, sp
        mov     ax, -1
        lar     cx, [bx+4]
        jnz     .no
        mov     al, ch
        xor     ah, ah
.no:    retf

; bpt(): breakpoint.
ENTRY bpt
        mov     word [_resume], .on
        int3
.on:    retf

; divzero(): divide by zero.
ENTRY divzero
        mov     word [_resume], .on
        xor     cx, cx
        mov     ax, 1
        xor     dx, dx
        div     cx
.on:    retf

; op386(): an instruction the 386 added (MOVZX AX, BL).
ENTRY op386
        mov     word [_resume], .on
        db      0x0F, 0xB6, 0xC3
.on:    retf

; loades(sel): load ES with a selector, and put it back.
ENTRY loades
        mov     bx, sp
        mov     word [_resume], .on
        push    es
        mov     es, [bx+4]
.on:    pop     es
        retf

; peek(sel, offset): read a word through a selector.
ENTRY peek
        mov     bx, sp
        mov     word [_resume], .on
        push    es
        mov     es, [bx+4]
        mov     bx, [bx+6]
        mov     ax, [es:bx]
.on:    pop     es
        retf

; poke(sel, offset, value): write one.
ENTRY poke
        mov     bx, sp
        mov     word [_resume], .on
        push    es
        mov     es, [bx+4]
        mov     ax, [bx+8]
        mov     bx, [bx+6]
        mov     [es:bx], ax
.on:    pop     es
        retf

; settask(): set the task-switched flag, as a context switch will.
ENTRY settask
        smsw    ax
        or      al, 8
        lmsw    ax
        retf

; cleartask(): clear it.
ENTRY cleartask
        clts
        retf
