; htons, ntohs, htonl, ntohl (NASM: small model, or with -DFAR medium).
; Network byte order puts the most significant byte first; the 286 puts
; it last.

%include "DEFS.inc"

        global  _htons, _ntohs, _htonl, _ntohl

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
