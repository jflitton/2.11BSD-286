; mcount (NASM), which counts a function's calls for prof(1): the 286's,
; after the PDP-11's (../../pdp/csu/mcount.s).  The small model's alone,
; whose IP is all of where a program is (profil(2)).
;
; A function compiled with cc -p calls it once it has its frame
; (lib/ccom/i286/c15.c), as the system call stubs of the C library for
; cc -p, libc_p.a, do at their beginning (../sys/mkstubs.c), with the
; address of the function's counter as its argument, two words of the
; data:
;
;	L5:	dw	0, _foo
;
; The first is where the function's count of calls is, a long, and 0
; until the first call.  That takes the next of the struct cnt that
; monitor() has made room for (mon.c), and puts there the second, the
; function's address; and each call adds one to the count.  Until
; monitor() has made room nothing is counted.  When there is no more
; it says so, once, and a function that has no count yet gets none.
; It changes AX, BX and CX, and that once what any call may.

%include "DEFS.inc"

        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _DATA _BSS

        extern  __countbase, __countend, _write
        global  mcount

        segment _TEXT
mcount:
        mov     bx, sp
        mov     bx, [bx+2]      ; the counter
        mov     ax, [bx]        ; where its count is
        or      ax, ax
        jz      .first
.count: mov     bx, ax
        add     word [bx], 1
        adc     word [bx+2], 0
        ret
.first: mov     ax, [__countbase] ; the next struct cnt
        or      ax, ax
        jz      .none           ; no room yet, or none left
        cmp     ax, [__countend]
        jae     .full
        mov     cx, [bx+2]      ; the function's address
        xchg    ax, bx
        mov     [bx], cx        ; is its pc
        lea     cx, [bx+2]      ; and this its ncall
        add     bx, 6
        mov     [__countbase], bx
        mov     bx, ax
        mov     [bx], cx
        mov     ax, cx
        jmp     .count
.full:  mov     word [__countbase], 0
        mov     ax, .end - .msg
        push    ax
        mov     ax, .msg
        push    ax
        mov     ax, 2
        push    ax
        call    _write          ; write(2, msg, sizeof msg)
        add     sp, 6
.none:  ret

        segment _DATA
.msg:   db      "mcount: counter overflow", 10
.end:
