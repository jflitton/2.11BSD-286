; htons, ntohs, htonl, ntohl (NASM, small model).  Network byte order
; puts the most significant byte first; the 286 puts it last.

        bits    16
        cpu     286

        segment _TEXT public align=2 class=CODE
        global  _htons, _ntohs, _htonl, _ntohl

_htons:
_ntohs: mov     bx, sp
        mov     ax, [bx+2]
        xchg    al, ah
        ret

_htonl:
_ntohl: mov     bx, sp
        mov     dx, [bx+2]      ; the low word becomes the high one
        mov     ax, [bx+4]
        xchg    al, ah
        xchg    dl, dh
        ret
