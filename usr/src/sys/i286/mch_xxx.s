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

; lldt(selector): make a segment the local descriptor table.
ENTRY lldt
        mov     bx, sp
        lldt    [bx+4]
        retf

; strlen(s)
;       char *s;
ENTRY strlen
        push    bp
        mov     bp, sp
        push    di
        push    ds
        pop     es
        mov     di, [bp+6]
        mov     cx, -1
        xor     al, al
        cld
        repne scasb
        mov     ax, -2
        sub     ax, cx
        pop     di
        pop     bp
        retf

; lgdtldt(): the descriptor for the local descriptor table has changed
; in the global table; take it up.
ENTRY lgdtldt
        mov     ax, GDT_LDT << 3
        lldt    ax
        retf

; The 287.  fpsave(area) stores its state in 94 bytes and leaves it
; initialized; fprestore(area) loads one; fpinit() starts a fresh one.
; fpts() makes the next 287 instruction trap (exception 7), which is how
; the kernel finds out that the running process uses it; fpclts() undoes
; that.
ENTRY fpsave
        mov     bx, sp
        mov     bx, [bx+4]
        fnsave  [bx]
        fwait
        retf

ENTRY fprestore
        mov     bx, sp
        mov     bx, [bx+4]
        frstor  [bx]
        retf

ENTRY fpinit
        fninit
        retf

ENTRY fpts
        smsw    ax
        or      al, 8
        lmsw    ax
        retf

ENTRY fpclts
        clts
        retf

; intsoff(): turn interrupts off, and return the flags as they were.
; intson(flags): put them back.
ENTRY intsoff
        pushf
        pop     ax
        cli
        retf

ENTRY intson
        mov     bx, sp
        push    word [bx+4]
        popf
        retf

; lar(selector): the access byte of the segment a selector names, or -1
; if it names nothing a process could reach.
ENTRY lar
        mov     bx, sp
        mov     bx, [bx+4]
        or      bl, 3
        lar     ax, bx
        jz      .ok
        mov     ax, -1
        retf
.ok:    mov     al, ah
        xor     ah, ah
        retf

; insque(element, after), remque(element): doubly linked queues whose
; elements begin with the forward and backward links.
ENTRY insque
ENTRY _insque
        mov     bx, sp
        push    si
        push    di
        mov     si, [bx+4]      ; element
        mov     di, [bx+6]      ; the one it follows
        mov     bx, [di]        ; the one it precedes
        mov     [si], bx
        mov     [si+2], di
        mov     [di], si
        mov     [bx+2], si
        pop     di
        pop     si
        retf

ENTRY remque
ENTRY _remque
        mov     bx, sp
        push    si
        mov     si, [bx+4]
        mov     bx, [si]        ; next
        mov     si, [si+2]      ; previous
        mov     [si], bx
        mov     [bx+2], si
        pop     si
        retf

; cpustop(): halt the processor, with interrupts off so that it stays so.
ENTRY cpustop
        cli
        hlt
        retf
