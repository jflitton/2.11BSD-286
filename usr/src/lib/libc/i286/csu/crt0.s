; C start-up (NASM): small model, with -DFAR medium, or with -DFAR and
; -DFARDATA large.  This must be the first file linked.
;
; The kernel starts a program (docs/processes.md) with DS = ES = SS its
; data segment and the stack pointer at the count of arguments; after it
; the pointers to the arguments, a null, the pointers to the environment
; and a null.  BX is the break: the program's stack lies between its data
; and the break, so it has no other way to know where its memory ends.

%include "DEFS.inc"

; The order of the segments here is their order in the program.  _NULL
; comes first in the data segment: nothing a program can point at may be
; at offset 0 there, where its address would be a null pointer.  The
; compiler's constants (CONST, CONST2) are of class DATA and go with
; _DATA.  _TEXT, which DEFS.inc declares, is the first code: in the
; medium model each C file's code is a segment of its own, NAME_TEXT, and
; the library's assembly shares _TEXT.
        segment _NULL public align=2 class=BEGDATA
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _NULL _DATA _BSS

        segment _NULL
        dw      0, 0            ; a null pointer to a string finds ""

        extern  _main, _exit, _strrchr
        global  _environ, ___progname, __curbrk, __minbrk

%ifndef FARDATA
        segment _DATA
___progname:
        dw      empty           ; the last part of the program's name
empty:  db      0

        segment _BSS
_environ:
        resw    1
__curbrk:                       ; the break, as brk() and sbrk() keep it
        resw    1
__minbrk:                       ; and where it started, below which it
        resw    1               ;   would cut into the stack

        segment _TEXT
..start:
        mov     [__curbrk], bx
        mov     [__minbrk], bx
        mov     bx, sp
        mov     ax, [bx]        ; argc
        lea     cx, [bx+2]      ; argv
        mov     dx, ax
        inc     dx
        shl     dx, 1
        add     dx, cx          ; envp, after argv and its null
        mov     [_environ], dx
        push    dx
        push    cx
        push    ax
        or      ax, ax
        jz      .call
        mov     bx, cx
        mov     bx, [bx]        ; argv[0]
        mov     [___progname], bx
        mov     ax, '/'
        push    ax
        push    bx
        CALLC   _strrchr
        add     sp, 4
        or      ax, ax
        jz      .call
        inc     ax
        mov     [___progname], ax
.call:  CALLC   _main
        push    ax
        CALLC   _exit           ; exit(main(argc, argv, envp))
.hang:  jmp     .hang

%else

; The large model.  Every pointer here is far, those the kernel left on
; the stack too: the arguments and the environment are in this segment,
; which is the program's own data, and their pointers say so.  A null
; pointer is selector 0, which nothing can be read through.
        segment _DATA
___progname:
        dw      empty, DGROUP
empty:  db      0

        segment _BSS
_environ:
        resd    1
__curbrk:
        resd    1
__minbrk:
        resd    1

        segment _TEXT
..start:
        mov     [__curbrk], bx
        mov     [__curbrk+2], ds
        mov     [__minbrk], bx
        mov     [__minbrk+2], ds
        mov     bp, sp
        mov     ax, [bp]        ; argc
        lea     cx, [bp+2]      ; argv
        mov     dx, ax
        inc     dx
        shl     dx, 2
        add     dx, cx          ; envp, after argv and its null
        mov     [_environ], dx
        mov     [_environ+2], ss
        push    ss
        push    dx
        push    ss
        push    cx
        push    ax
        or      ax, ax
        jz      .call
        les     bx, [bp+2]      ; argv[0]
        mov     [___progname], bx
        mov     [___progname+2], es
        mov     ax, '/'
        push    ax
        push    es
        push    bx
        CALLC   _strrchr
        add     sp, 6
        mov     cx, ax
        or      cx, dx
        jz      .call
        inc     ax
        mov     [ss:___progname], ax
        mov     [ss:___progname+2], dx
.call:  CALLC   _main
        push    ax
        CALLC   _exit           ; exit(main(argc, argv, envp))
.hang:  jmp     .hang
%endif
