; Leaving one flow of control in the kernel and taking up another.
;
; setjmp(label) remembers where it was called from: the registers a C
; function must preserve, the stack pointer and the return address.
; longjmp(p, label) goes back there, in process p, as if that setjmp had
; returned again, with 1 this time.
;
; label is always in the user structure of the process that calls, and
; the label to resume is the same member of process p's.  uswitch()
; (machdep.c) makes p's user structure and kernel stack the current ones:
; when it returns, uptr points at them, and the label is there at the
; offset it had here.

%include "mch_defs.inc"

        extern  _uswitch
        extern  _uptr

        TEXT

; setjmp(label)
;       label_t *label;
ENTRY setjmp
        mov     bx, sp
        mov     bx, [bx+4]
        mov     [bx], si
        mov     [bx+2], di
        mov     [bx+4], bp
        pop     ax
        pop     dx              ; return address
        mov     [bx+6], sp
        mov     [bx+8], ax
        mov     [bx+10], dx
        push    dx
        push    ax
        xor     ax, ax
        retf

; longjmp(p, label)
;       struct proc *p; label_t *label;
ENTRY longjmp
        mov     bx, sp
        mov     si, [bx+4]
        mov     di, [bx+6]
        sub     di, [_uptr]     ; where the label is in a user structure
        push    di
        push    si
        call    far _uswitch
        add     di, [_uptr]     ; the process's stack is not ours yet,
        mov     si, [di]        ;   but nothing here needs one
        mov     bp, [di+4]
        mov     sp, [di+6]
        push    word [di+10]
        push    word [di+8]
        mov     di, [di+2]
        mov     ax, 1
        retf

; A new process starts here: unew() made the label it is first resumed
; from say so, with the stack pointer at the frame it will go to its
; program by.
        extern  _childret, trapret
        global  _forkret
_forkret:
        mov     ax, sp
        push    ax
        call    far _childret
        pop     ax
        jmp     trapret
