; Copies: between the kernel and a process, between far addresses in the
; kernel, and between pieces of memory named by physical address.
;
; An address in a process is a selector and an offset (uaddr_t, seg.h).
; The routines that take one load the selector with its privilege bits
; set to ring 3, so the processor itself refuses a segment the process
; could not have used, and they let the processor's limit and
; write-protection checks find a bad offset or length.  Either way the
; result is a fault in the kernel, which trap() turns into a jump to the
; address in nofault.
;
; The kernel's side of each copy is a far pointer too, so that the same
; routines serve the kernel's data segment, a buffer, and anything else
; the kernel can address; a near pointer converts where the function is
; declared (seg.h).
;
; nofault is reached through SS, which is always the kernel's data
; segment, so that the network, whose DS is its own, can call these
; routines too (docs/network.md).

%include "mch_defs.inc"

        global  _nofault
        extern  _gdt:wrt DGROUP
; Nothing links to these: they are in the symbol table so that a profile
; (tools/kprof.py) names the code after each, not the function before it.
        global  copy, str, window

        BSS
_nofault:
        resw    1               ; where trap() sends a fault in the kernel

        TEXT

; copyin(from, to, count): copy from a process to the kernel.
;       uaddr_t from; void __far *to; u_int count;
; Returns 0, or EFAULT.
ENTRY copyin
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [ss:_nofault], cpfault
        les     di, [bp+10]
        mov     cx, [bp+14]
        mov     si, [bp+6]
        mov     ax, [bp+8]
        or      al, 3
        mov     ds, ax
        jmp     short copy

; copyout(from, to, count): copy from the kernel to a process.
;       void __far *from; uaddr_t to; u_int count;
; Returns 0, or EFAULT.
ENTRY copyout
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [ss:_nofault], cpfault
        mov     di, [bp+10]
        mov     cx, [bp+14]
        mov     ax, [bp+12]
        or      al, 3
        mov     es, ax
        lds     si, [bp+6]

copy:   cld
        shr     cx, 1
        rep movsw
        jnc     .even
        movsb
.even:  xor     ax, ax
done:   pop     ds
        mov     word [ss:_nofault], 0
        pop     di
        pop     si
        pop     bp
        retf

cpfault:
        mov     ax, EFAULT
        jmp     done

; bcopy(from, to, count): copy memory in the kernel.
;       void __far *from, *to; u_int count;
; The addresses are far so that this serves memory outside the kernel's
; data segment as well as in it.  Two pieces of one segment may overlap.
ENTRY bcopy
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        les     di, [bp+10]
        mov     cx, [bp+14]
        lds     si, [bp+6]
        mov     ax, ds
        mov     dx, es
        cmp     ax, dx
        jne     copy
        cmp     si, di
        jae     copy
        mov     ax, si
        add     ax, cx
        cmp     ax, di
        jbe     copy
        std                     ; overlapping, and moving upward: from
        add     si, cx          ;   the top down
        add     di, cx
        dec     si
        dec     di
        rep movsb
        cld
        xor     ax, ax
        jmp     done

; bzero(p, count): clear memory in the kernel.
;       void __far *p; u_int count;
ENTRY bzero
        push    bp
        mov     bp, sp
        push    di
        les     di, [bp+6]
        mov     cx, [bp+10]
        xor     ax, ax
        cld
        shr     cx, 1
        rep stosw
        jnc     .even
        stosb
.even:  pop     di
        pop     bp
        retf

; bcmp(a, b, count): compare memory in the kernel.
;       void __far *a, *b; u_int count;
; Returns 0 if the two are the same.
ENTRY bcmp
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        les     di, [bp+10]
        mov     cx, [bp+14]
        lds     si, [bp+6]
        xor     ax, ax
        cld
        jcxz    .same
        repe cmpsb
        je      .same
        inc     ax
.same:  pop     ds
        pop     di
        pop     si
        pop     bp
        retf

; copyinstr(from, to, max, done): copy a string from a process.
;       uaddr_t from; char __far *to; u_int max, *done;
; copyoutstr(from, to, max, done): copy one to a process.
;       char __far *from; uaddr_t to; u_int max, *done;
; copystr(from, to, max, done): copy one inside the kernel.
;       char __far *from, *to; u_int max, *done;
; Each copies up to and including the null, and stores the number of
; bytes copied through done if it is not zero.  Returns 0, ENOENT if
; there was no null in max bytes, or EFAULT.
ENTRY copyinstr
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [ss:_nofault], strfault
        les     di, [bp+10]
        mov     si, [bp+6]
        mov     ax, [bp+8]
        or      al, 3
        mov     ds, ax
        jmp     short str

ENTRY copyoutstr
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [ss:_nofault], strfault
        mov     di, [bp+10]
        mov     ax, [bp+12]
        or      al, 3
        mov     es, ax
        lds     si, [bp+6]
        jmp     short str

ENTRY copystr
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [ss:_nofault], strfault
        les     di, [bp+10]
        lds     si, [bp+6]

str:    mov     cx, [bp+14]
        mov     dx, cx
        cld
        jcxz    .long
.next:  lodsb
        stosb
        or      al, al
        loopnz  .next
        jnz     .long
        xor     ax, ax
        jmp     short strdone
.long:  mov     ax, ENOENT
strdone:
        pop     ds
        mov     word [ss:_nofault], 0
        sub     dx, cx
        mov     bx, [bp+16]
        or      bx, bx
        jz      .nocount
        mov     [bx], dx
.nocount:
        pop     di
        pop     si
        pop     bp
        retf

strfault:
        mov     ax, EFAULT
        jmp     strdone

; fubyte(addr), fuword(addr): fetch a byte or a word from a process.
;       uaddr_t addr;
; Return it, or -1 if it cannot be read.  The PDP-11's separate calls for
; instruction space are the same thing here.
ENTRY fubyte
ENTRY fuibyte
        mov     bx, sp
        mov     word [ss:_nofault], fufault
        mov     ax, [ss:bx+6]
        or      al, 3
        mov     es, ax
        mov     bx, [ss:bx+4]
        mov     al, [es:bx]
        xor     ah, ah
        mov     word [ss:_nofault], 0
        retf

ENTRY fuword
ENTRY fuiword
        mov     bx, sp
        mov     word [ss:_nofault], fufault
        mov     ax, [ss:bx+6]
        or      al, 3
        mov     es, ax
        mov     bx, [ss:bx+4]
        mov     ax, [es:bx]
        mov     word [ss:_nofault], 0
        retf

; subyte(addr, value), suword(addr, value): store one in a process.
; Return 0, or -1 if it cannot be written.
ENTRY subyte
ENTRY suibyte
        mov     bx, sp
        mov     word [ss:_nofault], fufault
        mov     ax, [ss:bx+6]
        or      al, 3
        mov     es, ax
        mov     al, [ss:bx+8]
        mov     bx, [ss:bx+4]
        mov     [es:bx], al
        xor     ax, ax
        mov     [ss:_nofault], ax
        retf

ENTRY suword
ENTRY suiword
        mov     bx, sp
        mov     word [ss:_nofault], fufault
        mov     ax, [ss:bx+6]
        or      al, 3
        mov     es, ax
        mov     ax, [ss:bx+8]
        mov     bx, [ss:bx+4]
        mov     [es:bx], ax
        xor     ax, ax
        mov     [ss:_nofault], ax
        retf

fufault:
        mov     word [ss:_nofault], 0
        mov     ax, -1
        retf

; Point a descriptor of the global table at memory: selector in BX, the
; physical address at [BP+SI].  The whole 64K from there is addressable.
window: mov     word [_gdt+bx], 0xFFFF
        mov     ax, [bp+si]
        mov     [_gdt+bx+2], ax
        mov     ax, [bp+si+2]
        mov     ah, SD_DATA
        mov     [_gdt+bx+4], ax
        ret

; pcopy(from, to, count): copy memory, by physical address.
;       long from, to; u_int count;
; pclear(addr, count): clear memory, by physical address.
;       long addr; u_int count;
; Neither may run past 64K from where it starts; count 0 is nothing.
; They use the two descriptors kept for them, so they are for the top
; half of the kernel only: an interrupt that used them would take the
; memory out from under the copy it interrupted.
ENTRY pcopy
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     bx, GDT_TMP1 << 3
        mov     si, 6
        call    window
        mov     bx, GDT_TMP2 << 3
        mov     si, 10
        call    window
        mov     cx, [bp+14]
        mov     ax, GDT_TMP2 << 3
        mov     es, ax
        mov     ax, GDT_TMP1 << 3
        mov     ds, ax
        xor     si, si
        xor     di, di
        jmp     copy

ENTRY pclear
        push    bp
        mov     bp, sp
        push    si
        push    di
        mov     bx, GDT_TMP2 << 3
        mov     si, 6
        call    window
        mov     cx, [bp+10]
        mov     ax, GDT_TMP2 << 3
        mov     es, ax
        xor     di, di
        xor     ax, ax
        cld
        shr     cx, 1
        rep stosw
        jnc     .even
        stosb
.even:  pop     di
        pop     si
        pop     bp
        retf

; pinsw(port, addr, count): read count words from an I/O port into
; memory, by physical address; poutsw(port, addr, count): write them.
;       int port; long addr; u_int count;
; Neither may run past 64K from where it starts.  They are for disk
; drivers, which move sectors to and from buffers and processes that
; they know only by address.  The descriptor they use is their own, so
; they may be called from an interrupt, but every caller must be at
; splbio or above: one could otherwise interrupt another.
ENTRY pinsw
        push    bp
        mov     bp, sp
        push    si
        push    di
        mov     bx, GDT_BIO << 3
        mov     si, 8
        call    window
        mov     ax, GDT_BIO << 3
        mov     es, ax
        xor     di, di
        mov     dx, [bp+6]
        mov     cx, [bp+12]
        cld
        rep insw
        pop     di
        pop     si
        pop     bp
        retf

ENTRY poutsw
        push    bp
        mov     bp, sp
        push    si
        push    ds
        mov     bx, GDT_BIO << 3
        mov     si, 8
        call    window
        mov     dx, [bp+6]
        mov     cx, [bp+12]
        mov     ax, GDT_BIO << 3
        mov     ds, ax
        xor     si, si
        cld
        rep outsw
        pop     ds
        pop     si
        pop     bp
        retf
