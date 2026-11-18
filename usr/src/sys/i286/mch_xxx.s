; Small routines for C.  Arguments are reached through BX without a
; frame, with SS, which is always the kernel's data segment: the network,
; whose DS is its own, calls some of these too (docs/network.md).

%include "mch_defs.inc"

        TEXT

; inb(port): read a byte from an I/O port.
ENTRY inb
        mov     bx, sp
        mov     dx, [ss:bx+4]
        in      al, dx
        xor     ah, ah
        retf

; outb(port, value): write a byte to an I/O port.
ENTRY outb
        mov     bx, sp
        mov     dx, [ss:bx+4]
        mov     al, [ss:bx+6]
        out     dx, al
        retf

; inw(port), outw(port, value): the same for 16 bits.
ENTRY inw
        mov     bx, sp
        mov     dx, [ss:bx+4]
        in      ax, dx
        retf

ENTRY outw
        mov     bx, sp
        mov     dx, [ss:bx+4]
        mov     ax, [ss:bx+6]
        out     dx, ax
        retf

; insb(port, buf, count), insw(port, buf, count): read count bytes, or
; words, from an I/O port into buf; outsb and outsw write them from it.
;       int port; char __far *buf; u_int count;
; For the network interfaces, whose buffers are the network's mbufs or
; its locals, on the stack: buf is far.
ENTRY insb
        push    bp
        mov     bp, sp
        push    di
        mov     dx, [bp+6]
        les     di, [bp+8]
        mov     cx, [bp+12]
        cld
        rep insb
        pop     di
        pop     bp
        retf

ENTRY insw
        push    bp
        mov     bp, sp
        push    di
        mov     dx, [bp+6]
        les     di, [bp+8]
        mov     cx, [bp+12]
        cld
        rep insw
        pop     di
        pop     bp
        retf

ENTRY outsb
        push    bp
        mov     bp, sp
        push    si
        push    ds
        mov     dx, [bp+6]
        lds     si, [bp+8]
        mov     cx, [bp+12]
        cld
        rep outsb
        pop     ds
        pop     si
        pop     bp
        retf

ENTRY outsw
        push    bp
        mov     bp, sp
        push    si
        push    ds
        mov     dx, [bp+6]
        lds     si, [bp+8]
        mov     cx, [bp+12]
        cld
        rep outsw
        pop     ds
        pop     si
        pop     bp
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
        mov     si, [ss:bx+4]   ; element
        mov     di, [ss:bx+6]   ; the one it follows
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
        mov     si, [ss:bx+4]
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
