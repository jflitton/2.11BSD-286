; Small routines for C.  The kernel's stack is in its data segment, so
; arguments are reached through BX without a frame.

%include "mch_defs.inc"

        TEXT

; inb(port): read a byte from an I/O port.
ENTRY inb
        mov     bx, sp
        mov     dx, [bx+4]
        in      al, dx
        xor     ah, ah
        retf

; outb(port, value): write a byte to an I/O port.
ENTRY outb
        mov     bx, sp
        mov     dx, [bx+4]
        mov     al, [bx+6]
        out     dx, al
        retf

; inw(port), outw(port, value): the same for 16 bits.
ENTRY inw
        mov     bx, sp
        mov     dx, [bx+4]
        in      ax, dx
        retf

ENTRY outw
        mov     bx, sp
        mov     dx, [bx+4]
        mov     ax, [bx+6]
        out     dx, ax
        retf

; enable(), disable(): let interrupts in, keep them out.
ENTRY enable
        sti
        retf

ENTRY disable
        cli
        retf

; idle(): wait for an interrupt.  STI takes effect after the next
; instruction, so an interrupt cannot slip in between the two.
ENTRY idle
        sti
        hlt
        retf

; lldt(selector): make a segment the local descriptor table.
ENTRY lldt
        mov     bx, sp
        lldt    [bx+4]
        retf
