; Exceptions and interrupts.
;
; Each of the NIDT vectors has a few instructions of its own, which leave
; the vector number and an error code on the stack and join the common
; path.  That saves the registers, makes DS and ES the kernel's data
; segment and calls
;
;       trap(frame)
;       struct trapframe *frame;
;
; The frame is on the kernel stack, which is in the kernel's data
; segment, so the pointer is an ordinary one.  What trap changes in the
; frame is what the interrupted code gets back.

%include "mch_defs.inc"

        extern  _trap
        global  _idt

        TEXT

; The processor pushes an error code itself for these five exceptions.
%define HASERR(v) (v = 8 || (v >= 10 && v <= 13))

%assign v 0
%rep NIDT
vec %+ v:
  %if HASERR(v)
  %else
        push    0
  %endif
        push    v
        jmp     alltraps
  %assign v v+1
%endrep

alltraps:
        push    ds
        push    es
        pusha
        mov     ax, DGROUP
        mov     ds, ax
        mov     es, ax
        cld
        mov     ax, sp
        push    ax
        call    far _trap
        pop     ax
        popa
        pop     es
        pop     ds
        add     sp, 4           ; the vector and the error code
        iret

; The interrupt descriptor table.  Every gate turns interrupts off.
        DATA
_idt:
%assign v 0
%rep NIDT
        dw      vec %+ v, seg vec %+ v
        db      0, SD_INTGATE
        dw      0
  %assign v v+1
%endrep
