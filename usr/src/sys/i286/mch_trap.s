; Exceptions, interrupts and system calls.
;
; Each of the NIDT vectors has a few instructions of its own, which leave
; the vector number and an error code on the stack and join the common
; path.  That saves the registers, makes DS and ES the kernel's data
; segment and calls
;
;       trap(frame)             or, for the system call vector,
;       syscall(frame)
;       struct trapframe *frame;
;
; The frame is on the kernel stack, which is in the kernel's data
; segment, so the pointer is an ordinary one.  What the function changes
; in the frame is what the interrupted code gets back.
;
; The registers are pushed one by one, not with PUSHA, to put the six
; that carry a system call's arguments next to each other in the order of
; the arguments.  syscall then has its argument list in place in the
; frame, with nothing fetched from the process and nothing copied.
;
; When a process is interrupted the processor first moves to the kernel
; stack named in the task state segment, which is empty then.  So a
; process's frame is always at the top of its kernel stack.

%include "mch_defs.inc"

        extern  _trap, _syscall
        global  _idt, trapret

        TEXT

; The processor pushes an error code itself for these five exceptions.
%define HASERR(v) (v = 8 || (v >= 10 && v <= 13))

%macro  SAVE 0
        push    ds
        push    es
        push    ax
        push    bp
        push    di
        push    si
        push    dx
        push    cx
        push    bx
        mov     ax, DGROUP
        mov     ds, ax
        mov     es, ax
        cld
%endmacro

%assign v 0
%rep NIDT
vec %+ v:
  %if HASERR(v)
  %else
        push    0
  %endif
        push    v
  %if v = T_SYSCALL
        jmp     allsys
  %else
        jmp     alltraps
  %endif
  %assign v v+1
%endrep

allsys: SAVE
        mov     ax, sp
        push    ax
        call    far _syscall
        jmp     short return

alltraps:
        SAVE
        mov     ax, sp
        push    ax
        call    far _trap
return: pop     ax
; Go where the frame at the top of the stack says.  The start-up code
; comes here to run the first process.
trapret:
        pop     bx
        pop     cx
        pop     dx
        pop     si
        pop     di
        pop     bp
        pop     ax
        pop     es
        pop     ds
        add     sp, 4           ; the vector and the error code
        iret

; The interrupt descriptor table.  Every gate turns interrupts off but the
; system call's.  A process may use that one and the two for the
; breakpoint and INTO instructions; INT with any other number is a
; protection fault.
        DATA
_idt:
%assign v 0
%rep NIDT
        dw      vec %+ v, seg vec %+ v
  %if v = T_SYSCALL
        db      0, SD_TRAPGATE | SD_USER
  %elif v = T_BPT || v = T_OFLOW
        db      0, SD_INTGATE | SD_USER
  %else
        db      0, SD_INTGATE
  %endif
        dw      0
  %assign v v+1
%endrep
