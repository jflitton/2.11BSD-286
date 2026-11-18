; Copies between the kernel and a process.
;
; An address in a process is a selector and an offset (uaddr_t, seg.h).
; These load the selector with its privilege bits set to ring 3, so the
; processor itself refuses a segment the process could not have used, and
; they let the processor's limit and write-protection checks find a bad
; offset or length.  Either way the result is a fault in the kernel,
; which trap() turns into a jump to the address in nofault.

%include "mch_defs.inc"

        global  _nofault

        BSS
_nofault:
        resw    1               ; where trap() sends a fault in the kernel

        TEXT

; copyin(from, to, count): copy from a process to the kernel.
;       uaddr_t from; caddr_t to; u_int count;
; Returns 0, or EFAULT.
ENTRY copyin
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [_nofault], cpfault
        push    ds
        pop     es
        mov     di, [bp+10]
        mov     cx, [bp+12]
        mov     si, [bp+6]
        mov     ax, [bp+8]
        or      al, 3
        mov     ds, ax
        jmp     short copy

; copyout(from, to, count): copy from the kernel to a process.
;       caddr_t from; uaddr_t to; u_int count;
; Returns 0, or EFAULT.
ENTRY copyout
        push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     word [_nofault], cpfault
        mov     si, [bp+6]
        mov     di, [bp+8]
        mov     cx, [bp+12]
        mov     ax, [bp+10]
        or      al, 3
        mov     es, ax

copy:   cld
        shr     cx, 1
        rep movsw
        jnc     .even
        movsb
.even:  xor     ax, ax
done:   pop     ds
        mov     word [_nofault], 0
        pop     di
        pop     si
        pop     bp
        retf

cpfault:
        mov     ax, EFAULT
        jmp     done
