; Exceptions, interrupts and system calls.
;
; Each of the NIDT vectors has a few instructions of its own, which leave
; the vector number and an error code on the stack and join the common
; path.  That saves the registers, makes DS and ES the kernel's data
; segment and calls
;
;       trap(frame)             for an exception,
;       syscall(frame)          for the system call vector, or
;       intr(frame)        for a device's interrupt request
;       struct trapframe *frame;
;
; The frame is on the kernel stack, which is in the kernel's data
; segment, so the pointer is an ordinary one.  What the function changes
; in the frame is what the interrupted code gets back.
;
; The registers are pushed one by one, not with PUSHA, to put the six
; that carry a system call's arguments next to each other in the order of
; the arguments: syscall has its argument list there in the frame, with
; nothing to fetch from the process.
;
; When a process is interrupted the processor first moves to the kernel
; stack named in the task state segment, which is empty then.  So a
; process's frame is always at the top of its kernel stack.

%include "mch_defs.inc"

        extern  _trap, _syscall, _intr, _ast, _spllower
        extern  _runrun, _cpl, _ipending, _spending, _noproc
        global  _idt, trapret, _intnest
        global  _tr_popes, _tr_popds, _tr_iret

        BSS
        resb    INTSTACK        ; the stack interrupts are handled on
intstack:
_intnest:
        resw    1               ; not zero: that stack is in use

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
  %elif v >= T_IRQ0 && v < T_IRQ0 + 16
        jmp     allintr
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
; If the frame is a process's, the three instructions with names can
; fault: on a selector that is no longer good, or a place to continue
; that is outside its segment.  trap() knows them by their addresses.
trapret:
        pop     bx
        pop     cx
        pop     dx
        pop     si
        pop     di
        pop     bp
        pop     ax
_tr_popes:
        pop     es
_tr_popds:
        pop     ds
        add     sp, 4           ; the vector and the error code
_tr_iret:
        iret

; A device wants service.  Its handler runs on a stack of the kernel's
; own, so that a process's kernel stack need not have room for every
; device's handler on top of whatever the process was doing; interrupts
; that arrive meanwhile stay on that stack.  intr() is called and
; returns with interrupts off, and turns them on in between.
;
; On the way back to a process, if something has asked for another
; process to run (runrun), ast() sees to that first, on the process's own
; kernel stack again and with interrupts on.
allintr:
        SAVE
        mov     bx, sp
        inc     word [_intnest]
        cmp     word [_intnest], 1
        jne     .on
        mov     sp, intstack
.on:    push    bx              ; where the frame is: kept, and the argument
        call    far _intr
        pop     sp
        dec     word [_intnest]
        jnz     trapret
        mov     bx, sp
        test    byte [bx+TF_CS], 3
        jz      trapret
        cmp     word [_runrun], 0
        je      trapret
        sti
        push    bx
        call    far _ast
        pop     ax
        cli
        jmp     trapret

; Interrupt priority.  cpl is the level; an interrupt whose level is not
; above it is put off by intr(), which notes it in ipending.  These
; raise the level, and return what it was for splx to put back.
;
; The network calls these too, with DS its own data segment, so they
; reach the kernel's data through SS, which is always the kernel's.
%macro  SPL 2
ENTRY %1
ENTRY _%1
        mov     ax, %2
        jmp     short raise
%endmacro

        SPL     splsoftclock, IPL_SOFTCLOCK
        SPL     splnet, IPL_NET
        SPL     splbio, IPL_BIO
        SPL     splimp, IPL_IMP
        SPL     spltty, IPL_TTY
        SPL     spl5, IPL_TTY
        SPL     splclock, IPL_CLOCK
        SPL     splhigh, IPL_HIGH

raise:  cmp     ax, [ss:_cpl]
        jbe     .keep
        xchg    ax, [ss:_cpl]
        retf
.keep:  mov     ax, [ss:_cpl]
        retf

; spl0(): let everything in.  splx(s): put the level back to s.  If
; something was put off meanwhile, spllower() runs what the new level
; allows, on the interrupt stack as if it had just arrived.
ENTRY spl0
ENTRY _spl0
        xor     ax, ax
        jmp     short lower
ENTRY splx
        mov     bx, sp
        mov     ax, [ss:bx+4]
lower:  xchg    ax, [ss:_cpl]
        mov     bx, [ss:_ipending]
        or      bx, [ss:_spending]
        jnz     .run
        retf
.run:   push    ax
        pushf
        cli
        push    ds
        push    ss
        pop     ds
        call    runpend
        pop     ds
        popf
        pop     ax
        retf

; Call spllower() on the interrupt stack, unless already on it.  Entered
; and left with interrupts off, and with DS the kernel's.
runpend:
        mov     bx, sp
        inc     word [_intnest]
        cmp     word [_intnest], 1
        jne     .on
        mov     sp, intstack
.on:    push    bx
        call    far _spllower
        pop     sp
        dec     word [_intnest]
        ret

; idle(): there is nothing to run; wait for an interrupt.  Whatever level
; the caller is at, everything is let in meanwhile, as if by spl0.  If
; nothing was put off, the processor halts: STI takes effect after the
; next instruction, so an interrupt cannot slip in between the two and
; leave the processor halted with work to do.  noproc says that no
; process is running, so that the clock charges the time to none (swtch
; clears it).
ENTRY idle
        cli
        mov     word [_noproc], 1
        xor     ax, ax
        xchg    ax, [_cpl]
        push    ax
        mov     bx, [_ipending]
        or      bx, [_spending]
        jz      .wait
        call    runpend
        sti
        jmp     short .out
.wait:  sti
        hlt
.out:   pop     ax
        mov     [_cpl], ax
        retf

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
