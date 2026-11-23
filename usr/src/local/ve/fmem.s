; fcopy and fmove: copy bytes between far addresses, for ve (NASM, with
; the C library's conventions: small model, or with -DFAR medium).  The
; text ve edits is in segments outside the program's own (text.c), and C
; that moved it a byte at a time through far pointers would load a
; segment register for each.
;
;       fcopy(from, to, n)      copy n bytes, from the first on
;       fmove(from, to, n)      the same where the two may overlap: if
;                               to is the higher, from the last back
;       fscan(p, c, n)          how many of the n bytes at p are before
;                               the first that is c: n if none is
;       fin(fd, p, n)           read(2) and write(2) with a far p, which
;       fout(fd, p, n)          the library's take only in the large
;                               model: the blocks of text go between
;                               their segments and a file as they are
;
; from, to and p are far pointers, an offset and then a selector.  The
; last is for the two places ve looks at every byte, a file as it is
; read and a line as it is searched, where C would take ten times as
; long over each.

%include "DEFS.inc"

        global  _fcopy, _fmove, _fscan, _fin, _fout
        extern  cerror

SRC     equ     2+RA
DST     equ     2+RA+4
CNT     equ     2+RA+8

_fmove: push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     cx, [bp+CNT]
        les     di, [bp+DST]
        lds     si, [bp+SRC]
        mov     ax, ds
        mov     dx, es
        cmp     ax, dx
        jne     fwd
        cmp     di, si
        jbe     fwd
        add     si, cx
        add     di, cx
        dec     si
        dec     di
        std
        rep     movsb
        cld
        jmp     done

_fcopy: push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        mov     cx, [bp+CNT]
        les     di, [bp+DST]
        lds     si, [bp+SRC]
fwd:    cld
        shr     cx, 1
        rep     movsw
        jnc     done
        movsb
done:   pop     ds
        pop     di
        pop     si
        pop     bp
        RET

_fscan: push    bp
        mov     bp, sp
        push    di
        les     di, [bp+2+RA]
        mov     ax, [bp+2+RA+4]
        mov     cx, [bp+2+RA+6]
        mov     dx, cx
        jcxz    none
        cld
        repne   scasb
        jne     none
        mov     ax, dx
        sub     ax, cx
        dec     ax
        jmp     found
none:   mov     ax, dx
found:  pop     di
        pop     bp
        RET

; A system call's number goes in AX and its arguments in BX, CX, DX and
; SI, a pointer as its offset and then its selector; the carry says
; that it failed, and cerror makes errno of what AX has then.
_fin:   mov     ax, 3
        jmp     rw
_fout:  mov     ax, 4
rw:     push    bp
        mov     bp, sp
        push    si
        push    di
        mov     bx, [bp+2+RA]
        mov     cx, [bp+2+RA+2]
        mov     dx, [bp+2+RA+4]
        mov     si, [bp+2+RA+6]
        int     0x30
        pop     di
        pop     si
        pop     bp
        jnc     rwok
%ifdef FAR
        jmp     far cerror
%else
        jmp     cerror
%endif
rwok:   RET
