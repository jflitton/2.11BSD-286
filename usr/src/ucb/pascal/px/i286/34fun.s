;
; BUILT IN FUNCTIONS
;
_LLIMIT:
        add     sp, 4
        NEXT
_ARGC:
        mov     ax, [_argc]
        cwd
        push    dx
        push    ax
        NEXT
_ARGV:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        pop     di
        pop     ax
        test    ax, ax
        jl      .9
        cmp     ax, [_argc]
        jge     .9
        mov     bx, ax
        add     bx, bx
        add     bx, [_argv]
        mov     bx, [bx]
        jcxz    .4
.2:
        mov     al, [bx]
        inc     bx
        test    al, al
        jnz     .3
        dec     bx
        mov     al, ' '
.3:
        mov     [di], al
        inc     di
        loop    .2
.4:
        NEXT
.9:
        ERROR   EARGV
;
; The times of times(), user or system, in milliseconds
;
_SCLCK:
        mov     di, 4
        jmp     clck
_CLCK:
        xor     di, di
clck:
        sub     sp, 16
        mov     ax, sp
        push    ax
        call    _times
        add     sp, 2
        mov     bx, sp
        fild    dword [bx+di]
        fidiv   word [hz]
        fimul   word [thousand]
        add     sp, 12
        jmp     fistl
_DATE:
_TIME:
        mov     ax, bx
        sar     ax, 1
        and     ax, 0xFF
        push    ax
        call    _pdattim
        add     sp, 4
        NEXT
_SEED:
        pop     ax
        pop     dx
        push    word [_seed+2]
        push    word [_seed]
        mov     [_seed], ax
        mov     [_seed+2], dx
        NEXT
;
; The fraction of (seed * randa + randc) * randim is the number, and
; that times randm the next seed.
;
_RANDOM:
        fild    dword [_seed]
        fmul    qword [_randa]
        fadd    qword [_randc]
        fmul    qword [_randim]
        fld     st0
        call    __CHP
        fsubp   st1, st0
        mov     bx, sp
        fst     qword [bx]
        fmul    qword [_randm]
        call    __CHP
        fistp   dword [_seed]
        fwait
        NEXT
_DISPOSE:
        mov     bx, sp
        cmp     word [bx], 0
        je      .1
        call    _pfree
.1:
        add     sp, 2
        NEXT
_NEW:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        push    cx
        call    _alloc
        add     sp, 2
        pop     bx
        mov     [bx], ax
        NEXT
;
; The exponent of the real, as the PDP-11's has it: its fraction is
; from 1/2 to 1, so the exponent is one more than the 287's, and 0's is
; -128.
;
_EXPO:
        mov     bx, sp
        mov     ax, [bx+6]
        and     ax, 0x7FFF
        or      ax, [bx+4]
        or      ax, [bx+2]
        or      ax, [bx]
        mov     ax, -128
        jz      .1
        fld     qword [bx]
        fxtract
        fstp    qword [bx]
        fistp   word [bx]
        fwait
        mov     ax, [bx]
        inc     ax
.1:
        add     sp, 8
        cwd
        push    dx
        push    ax
        NEXT
_ATAN:
        call    _atan
        jmp     fret
_COS:
        call    _cos
        jmp     fret
_EXP:
        call    _exp
        jmp     fret
_LN:
        mov     bx, sp
        test    byte [bx+7], 0x80
        jnz     .9
        mov     ax, [bx+6]
        or      ax, [bx+4]
        or      ax, [bx+2]
        or      ax, [bx]
        jz      .9
        call    _log
        jmp     fret
.9:
        ERROR   ELN
_SIN:
        call    _sin
        jmp     fret
;
; The square root of -0 is -0, as the 287's is; that of a real less
; than 0 is an error.
;
_SQRT:
        mov     bx, sp
        test    byte [bx+7], 0x80
        jz      .1
        mov     ax, [bx+6]
        and     ax, 0x7FFF
        or      ax, [bx+4]
        or      ax, [bx+2]
        or      ax, [bx]
        jnz     .9
.1:
        call    _sqrt
        jmp     fret
.9:
        ERROR   ESQRT
;
; fret: the real the C function gave back, in place of its argument
;
fret:
        mov     bx, sp
        fstp    qword [bx]
        fwait
        NEXT
_CHR4:
        pop     ax
        mov     bx, sp
        cmp     word [bx], 0
        mov     [bx], ax
        jne     chrerr
_CHR2:
        mov     bx, sp
        cmp     word [bx], 127
        ja      chrerr
        NEXT
chrerr:
        ERROR   ECHR
_ODD4:
        pop     ax
        mov     bx, sp
        mov     [bx], ax
_ODD2:
        mov     bx, sp
        and     word [bx], 1
        NEXT
_PRED2:
        mov     bx, sp
        dec     word [bx]
        NEXT
_PRED4:
        mov     bx, sp
        sub     word [bx], 1
        sbb     word [bx+2], 0
        NEXT
_PRED24:
        pop     ax
        dec     ax
        cwd
        push    dx
        push    ax
        NEXT
_STLIM:
        pop     ax
        pop     dx
        test    dx, dx
        js      .2
        sub     ax, [_stcnt]
        sbb     dx, [_stcnt+2]
        jns     .1
        ERROR   ESTLIM
.1:
        mov     [_stlim], ax
        mov     [_stlim+2], dx
.2:
        NEXT
_SUCC2:
        mov     bx, sp
        inc     word [bx]
        NEXT
_SUCC4:
        mov     bx, sp
        add     word [bx], 1
        adc     word [bx+2], 0
        NEXT
_SUCC24:
        pop     ax
        inc     ax
        cwd
        push    dx
        push    ax
        NEXT
_ROUND:
        mov     bx, sp
        fld     qword [bx]
        test    byte [bx+7], 0x80
        jnz     .1
        fadd    qword [half]
        jmp     .2
.1:
        fsub    qword [half]
.2:
        add     sp, 4
        jmp     fistl
_TRUNC:
        mov     bx, sp
        fld     qword [bx]
        add     sp, 4
        jmp     fistl
_UNDEF:
        add     sp, 8
        push    word 0
        NEXT
;
; pack(a,i,z)
;
; with:	a: array[m..n] of t
;	z: packed array[u..v] of t
;
; semantics:	for j := u to v do
;			z[j] := a[j-u+i];
;
; need to check:
;	1. i >= m
;	2. i+(v-u) <= n		(i.e. i-m <= (n-m)-(v-u))
;
; on stack:	lv(z), lv(a), rv(i) (len 2)
;
; move w(t)*(v-u+1) bytes from lv(a)+w(t)*(i-m) to lv(z)
;
; CX is w(t), and BP 0 for pack and 1 for unpack, where the PDP-11's
; had where to go after in reta.
;
_PACK:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        xor     bp, bp
        jmp     pakunp
;
; unpack(z,a,i)
;
; with:	z and a as in pack
;
; semantics:	for j := u to v do
;			a[j-u+i] := z[j]
;
_UNPACK:
        jnz     .1
        lodsw
        mov     cx, ax
.1:
        mov     bp, 1
;
; check conditions 1 and 2
;
pakunp:
        mov     bx, sp
        mov     ax, [bx+4]
        sub     ax, [si]                ; m
        jl      .9
        cmp     ax, [si+2]              ; (n-m)-(u-v)
        jg      .9
        imul    cx
        pop     di                      ; lv(z)
        pop     bx
        add     bx, ax                  ; lv(a)+w(t)*(i-m)
        add     sp, 2
        mov     cx, [si+4]
        add     si, 6
        test    bp, bp
        jz      .1
        xchg    bx, di
.1:
        mov     ax, ds
        mov     es, ax
        mov     ax, si
        mov     si, bx
        rep movsb
        mov     si, ax
        NEXT
.9:
        test    bp, bp
        jnz     .8
        ERROR   EPACK
.8:
        ERROR   EUNPACK
_WCLCK:
        push    word 0
        call    _time
        add     sp, 2
        push    dx
        push    ax
        NEXT

        segment _DATA
half:   dd      0x00000000, 0x3FE00000  ; 0.5
hz:     dw      HZ
thousand: dw    1000

        segment _TEXT
