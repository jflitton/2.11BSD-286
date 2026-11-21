; The way into t_nd.c, compiled with -nd=ND, whose DS is ND_GROUP and
; not SS (tools/ncc -nd): t_nd makes DS that and calls it, and its
; checks come back by way of ndcheck and ndlcheck, which give
; support.c's check and lcheck their DS, which is SS here, back.

        bits    16
        cpu     286

        segment _TEXT public align=2 class=CODE
        segment ND_DATA public align=2 class=FAR_DATA
        segment ND_BSS public align=2 class=FAR_DATA
        group   ND_GROUP ND_DATA ND_BSS

        extern  _nd_t_nd, _check, _lcheck
        global  _t_nd, _ndcheck, _ndlcheck

        segment _TEXT

_t_nd:  push    ds
        mov     ax, ND_GROUP
        mov     ds, ax
        call    _nd_t_nd
        pop     ds
        ret

; ndcheck(line, got, want)
_ndcheck:
        push    bp
        mov     bp, sp
        push    ds
        push    ss
        pop     ds
        push    word [bp+8]
        push    word [bp+6]
        push    word [bp+4]
        call    _check
        add     sp, 6
        pop     ds
        pop     bp
        ret

; ndlcheck(line, got, want), got and want long
_ndlcheck:
        push    bp
        mov     bp, sp
        push    ds
        push    ss
        pop     ds
        push    word [bp+12]
        push    word [bp+10]
        push    word [bp+8]
        push    word [bp+6]
        push    word [bp+4]
        call    _lcheck
        add     sp, 10
        pop     ds
        pop     bp
        ret
