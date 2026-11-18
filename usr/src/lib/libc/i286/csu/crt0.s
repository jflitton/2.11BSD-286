; C start-up (NASM): small model, or with -DFAR medium.  This must be the
; first file linked.
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
