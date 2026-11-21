; The Internet checksum (RFC 1071) over the first len bytes of a chain of
; mbufs: the ones' complement of the ones' complement sum of the data as
; 16-bit words.
;
;       in_cksum(m, len)
;       struct mbuf *m; int len;
;
; On a little-endian machine the sum of the words as the processor loads
; them is the byte-swapped sum of them in network order, so the result,
; stored back the same way, is right as it stands.
;
; The sum is kept in 16 bits, each carry out of the top added back in at
; the bottom by the next ADC.  An mbuf may hold an odd number of bytes:
; its last one is the low half of a word whose high half is the next
; mbuf's first.  Swapping the two bytes of a ones' complement sum gives
; the sum of the same words with their bytes swapped, so after an odd
; mbuf the sum is swapped and what follows is added as if it began on a
; word; a second odd one swaps it back, and so does the end.
;
; Most of the network's time moving data is spent here: 16 words are
; added in 16 instruction pairs with no test between them.
;
; The network calls this, with DS its own data segment, where mbufs are
; (docs/network.md).  in_cksum.c is the same in C, the reference.

%include "mch_defs.inc"

        extern  _netprintf

        segment NET_DATA public align=2 class=FAR_DATA
        segment NET_BSS public align=2 class=FAR_DATA
        group   NET_GROUP NET_DATA NET_BSS

; struct mbuf (h/mbuf.h)
M_NEXT          equ     0
M_OFF           equ     2
M_LEN           equ     4

        segment NET_DATA
nodata: db      "cksum: out of data", 10, 0

        TEXT

; htons(x), ntohs(x), htonl(x), ntohl(x): the bytes of a short or a long
; the other way round.  The cross compiler has them in line (machparam.h)
; and the system's own compiler calls these.
ENTRY htons
ENTRY ntohs
        mov     bx, sp
        mov     ax, [ss:bx+4]
        xchg    al, ah
        retf

ENTRY htonl
ENTRY ntohl
        mov     bx, sp
        mov     ax, [ss:bx+6]   ; the high word is the low one now
        mov     dx, [ss:bx+4]
        xchg    al, ah
        xchg    dl, dh
        retf

ENTRY in_cksum
        push    bp
        mov     bp, sp
        push    si
        push    di
        mov     bx, [bp+6]      ; the mbuf
        mov     di, [bp+8]      ; bytes still to add
        xor     dx, dx          ; the sum
        xor     bp, bp          ; not zero: the sum is swapped
        cld
.mbuf:  test    di, di
        jle     .end
        test    bx, bx
        jz      .short
        mov     cx, [bx+M_LEN]
        cmp     cx, di
        jle     .all
        mov     cx, di
.all:   sub     di, cx
        mov     si, [bx+M_OFF]
        add     si, bx
        mov     bx, [bx+M_NEXT]
        shr     cx, 1           ; words, and the carry: a byte after them
        pushf
        jz      .byte
; The first time round the 16 pairs does the words over a multiple of 16,
; by starting that many pairs from the end; a pair is 3 bytes.  LOOP
; leaves the carry alone.
        mov     ax, cx
        add     cx, 15
        shr     cx, 4           ; times round
        neg     ax
        and     ax, 15          ; pairs to skip
        imul    ax, ax, 3
        add     ax, .words
        clc
        jmp     ax
.words:
%rep 16
        lodsw
        adc     dx, ax
%endrep
        loop    .words
        adc     dx, 0
.byte:  popf
        jnc     .mbuf
        lodsb
        xor     ah, ah
        add     dx, ax
        adc     dx, 0
        xchg    dl, dh
        not     bp
        jmp     .mbuf

.short: push    dx
        push    nodata
        call    far _netprintf
        pop     ax
        pop     dx
.end:   test    bp, bp
        jz      .done
        xchg    dl, dh
.done:  mov     ax, dx
        not     ax
        pop     di
        pop     si
        pop     bp
        retf
