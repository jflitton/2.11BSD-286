; setjmp, longjmp, _setjmp and _longjmp (NASM): small model, or with
; -DFAR medium.
;
; A jmp_buf is a struct sigcontext (<signal.h>, <setjmp.h>).  setjmp
; fills in all of it: the signal mask and whether the signal stack is in
; use, as well as the registers its caller will have when it returns.
; longjmp has the kernel resume that context with sigreturn, which puts
; back the mask and every register at once.  _setjmp and _longjmp leave
; the signals alone, and only save and restore what the calling
; convention says a function keeps: SI, DI, BP, SP, and where to go.
;
; Where to go is the return address: in the medium model a far one,
; whose segment is the context's CS.

%include "DEFS.inc"

; struct sigcontext
SC_ONSTACK equ  0
SC_MASK equ     2
SC_AX   equ     6
SC_SI   equ     14
SC_DI   equ     16
SC_BP   equ     18
SC_DS   equ     20
SC_ES   equ     22
SC_IP   equ     24
SC_CS   equ     26
SC_FLAGS equ    28
SC_SP   equ     30
SC_SS   equ     32

SA_ONSTACK equ  1
SIG_BLOCK equ   1

        extern  __sigaltstack, _sigprocmask, _sigreturn, _longjmperror
        global  _setjmp, _longjmp, __setjmp, __longjmp

ENV     equ     2+RA            ; the argument env, from BP

; setjmp(env)
_setjmp:
        push    bp
        mov     bp, sp
        push    si
        push    di
        sub     sp, 8           ; the kernel's struct sigaltstack, at BP-12
        mov     ax, sp
        push    ax              ; _sigaltstack(0, &oss)
        xor     ax, ax
        push    ax
        CALLC   __sigaltstack
        mov     bx, [bp+ENV]
        mov     ax, [bp-6]      ; its ss_flags
        and     ax, SA_ONSTACK
        mov     [bx+SC_ONSTACK], ax
        lea     ax, [bx+SC_MASK]
        push    ax              ; sigprocmask(SIG_BLOCK, 0, &env->sc_mask)
        xor     ax, ax
        push    ax
        mov     ax, SIG_BLOCK
        push    ax
        CALLC   _sigprocmask
        lea     sp, [bp-4]
        mov     bx, [bp+ENV]
        mov     [bx+SC_SI], si
        mov     [bx+SC_DI], di
        mov     ax, [bp]
        mov     [bx+SC_BP], ax
        mov     [bx+SC_DS], ds
        mov     [bx+SC_ES], ds  ; ES may hold anything; DS is good
        mov     ax, [bp+2]
        mov     [bx+SC_IP], ax
%ifdef FAR
        mov     ax, [bp+4]
        mov     [bx+SC_CS], ax
%else
        mov     [bx+SC_CS], cs
%endif
        pushf
        pop     ax
        mov     [bx+SC_FLAGS], ax
        lea     ax, [bp+ENV]    ; the stack pointer after the return
        mov     [bx+SC_SP], ax
        mov     [bx+SC_SS], ss
        xor     ax, ax
        pop     di
        pop     si
        pop     bp
        RET

; longjmp(env, val): setjmp returns val, or 1 if val is 0.
_longjmp:
        mov     bx, sp
        mov     ax, [bx+RA+2]
        mov     bx, [bx+RA]
        or      ax, ax
        jnz     .val
        inc     ax
.val:   mov     [bx+SC_AX], ax
        push    bx
        CALLC   _sigreturn
        CALLC   _longjmperror   ; the kernel refused the context
        hlt                     ; and there is nowhere to go: fault

; _setjmp(env)
__setjmp:
        mov     bx, sp
        mov     cx, [bx]        ; the return address
%ifdef FAR
        mov     ax, [bx+2]
%else
        mov     ax, cs
%endif
        lea     dx, [bx+RA]     ; the stack pointer after the return
        mov     bx, [bx+RA]
        mov     [bx+SC_SI], si
        mov     [bx+SC_DI], di
        mov     [bx+SC_BP], bp
        mov     [bx+SC_IP], cx
        mov     [bx+SC_CS], ax
        mov     [bx+SC_SP], dx
        mov     [bx+SC_SS], ss
        mov     [bx+SC_DS], ds
        xor     ax, ax
        RET

; _longjmp(env, val)
__longjmp:
        mov     bx, sp
        mov     ax, [bx+RA+2]
        mov     bx, [bx+RA]
        or      ax, ax
        jnz     .val
        inc     ax
.val:   mov     si, [bx+SC_SI]
        mov     di, [bx+SC_DI]
        mov     bp, [bx+SC_BP]
        mov     sp, [bx+SC_SP]
%ifdef FAR
        jmp     far [bx+SC_IP]  ; SC_CS follows it
%else
        jmp     word [bx+SC_IP]
%endif
