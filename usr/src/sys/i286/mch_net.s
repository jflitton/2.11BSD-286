; Calls between the kernel and the network (net_mac.h, docs/network.md).
;
; The network's code is compiled with a data segment of its own,
; NET_GROUP, and runs on the kernel's stacks: in it DS is NET_GROUP and SS
; the kernel's DGROUP, and it reaches the kernel's data through SS.  These
; routines are the only ways between the two:
;
;       KScall(func, nbytes, args...)   the kernel calls the network
;       SKcall(func, nbytes, args...)   the network calls the kernel
;               int (*func)(); u_int nbytes;
;
; Each pushes the nbytes of arguments again as a frame for func, makes DS
; the callee's for the call, and on the way back puts the caller's DS
; back; func's result comes back in AX, or DX:AX.
;
; netuptr is the network's copy of uptr, through which it sees u.
; KScall sets it, from the running process's, and SKcall puts it back
; after a call, in which other processes may have run and called the
; network themselves; each puts back the value it found, so a call that
; arrives in an interrupt leaves it as it was.  The network never
; touches u at an interrupt's level.

%include "mch_defs.inc"

        extern  _uptr:wrt DGROUP, _spending:wrt DGROUP
        extern  _time:wrt DGROUP, _lbolt:wrt DGROUP, _selwait:wrt DGROUP
        extern  _cfreecount:wrt DGROUP

        segment NET_DATA public align=2 class=FAR_DATA
        segment NET_BSS public align=2 class=FAR_DATA
        group   NET_GROUP NET_DATA NET_BSS

; The kernel's variables that the network reads, through SS: their
; addresses in the kernel's data segment, which the network's own code
; cannot name (net_mac.h).
        segment NET_DATA
        global  _ktime, _klbolt, _kselwait, _kcfreecount
_ktime:         dw      _time
_klbolt:        dw      _lbolt
_kselwait:      dw      _selwait
_kcfreecount:   dw      _cfreecount

        segment NET_BSS
        global  _netuptr
_netuptr:
        resw    1

        TEXT

ENTRY KScall
        push    bp
        mov     bp, sp
        push    si
        push    ds
        mov     ax, NET_GROUP
        mov     ds, ax
        push    word [_netuptr]
        mov     ax, [ss:_uptr]
        mov     [_netuptr], ax
        call    args
        call    far [bp+6]
        add     sp, [bp+10]
        pop     word [_netuptr]
        pop     ds
        pop     si
        pop     bp
        retf

ENTRY SKcall
        push    bp
        mov     bp, sp
        push    si
        push    ds
        push    word [_netuptr]
        call    args
        push    ss
        pop     ds
        call    far [bp+6]
        add     sp, [bp+10]
        mov     ds, [bp-4]
        pop     word [_netuptr]
        pop     ds
        pop     si
        pop     bp
        retf

; Push the arguments of the call whose frame BP points at, the last
; first.  Leaves them below its own return address, which it takes with
; it.
args:   pop     cx
        mov     si, [bp+10]
.next:  sub     si, 2
        jb      .done
        push    word [bp+12+si]
        jmp     short .next
.done:  jmp     cx

; setsoftnet(): have netintr run once the level drops below splnet, as a
; software interrupt (machdep.c).  For the network, from any level.
ENTRY setsoftnet
        or      word [ss:_spending], SP_NET
        retf

; mfsd(addr): the word at addr in the network's data.  mtsd(addr, w): set
; it.  For the kernel, which keeps the network's addresses but cannot
; follow them otherwise.
;       int *addr; int w;
ENTRY mfsd
        mov     bx, sp
        push    ds
        mov     ax, NET_GROUP
        mov     ds, ax
        mov     bx, [ss:bx+4]
        mov     ax, [bx]
        pop     ds
        retf

ENTRY mtsd
        mov     bx, sp
        push    ds
        mov     ax, NET_GROUP
        mov     ds, ax
        mov     ax, [ss:bx+6]
        mov     bx, [ss:bx+4]
        mov     [bx], ax
        pop     ds
        retf
