; htons, ntohs, htonl, ntohl (NASM: small model, with -DFAR medium, or
; with -DFARDATA too large).
; Network byte order puts the most significant byte first; the 286 puts
; it last.

%include "DEFS.inc"

        global  _htons, _ntohs, _htonl, _ntohl

%ifndef FARDATA
_htons:
_ntohs: mov     bx, sp
        mov     ax, [bx+RA]
        xchg    al, ah
        RET

_htonl:
_ntohl: mov     bx, sp
        mov     dx, [bx+RA]     ; the low word becomes the high one
        mov     ax, [bx+RA+2]
        xchg    al, ah
        xchg    dl, dh
        RET
%else
; The large model: DS is not the stack's segment.
_htons:
_ntohs: mov     bx, sp
        mov     ax, [ss:bx+RA]
        xchg    al, ah
        RET

_htonl:
_ntohl: mov     bx, sp
        mov     dx, [ss:bx+RA]
        mov     ax, [ss:bx+RA+2]
        xchg    al, ah
        xchg    dl, dh
        RET
%endif
