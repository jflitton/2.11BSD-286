; Where the kernel starts.
;
; The boot program has loaded the kernel's segments and enters here in
; real mode (docs/boot.md):
;
;       DS      where the kernel's data segment was loaded
;       ES:SI   struct bootinfo: where every segment was loaded
;
; This asks the BIOS how much memory there is, opens address line 20,
; describes the kernel's segments in the global descriptor table, moves
; the interrupt controllers' vectors clear of the processor's exceptions,
; enters protected mode and calls main, and when main returns starts the
; first process.  Everything before the mode switch uses near jumps and
; calls only: the far addresses in the kernel are protected-mode
; selectors, which mean nothing yet.
;
; The stack is the kernel stack of the first slot (uslot, param.c), which
; becomes the swapper's; uptr, the running process's user structure, is
; set to match.

%include "mch_defs.inc"

        extern  _main, trapret
        extern  _idt:wrt DGROUP
        extern  _uslot:wrt DGROUP, _uptr:wrt DGROUP
        global  _bootinfo, _basemem, _extmem, _gdt, _ktss
        global  pmode           ; where a process's kernel stack begins,
                                ; which adb's $c names; the cross
                                ; build's kernel has global names alone

        BSS
_bootinfo:
        resb    BI_SIZE
_basemem:
        resw    1               ; K of memory below 640K
_extmem:
        resw    1               ; K of memory above 1M
_gdt:   resb    NGDT * 8
_ktss:  resb    TS_SIZE
        alignb  2
gdtptr: resb    6               ; limit and physical address, for LGDT
idtptr: resb    6
mvgdt:  resb    48              ; descriptors for the BIOS's block move
rdspt:  resw    1               ; the boot drive: sectors a track,
rdheads:
        resw    1               ;   and heads

        TEXT
..start:
        cld
        mov     ax, ds
        push    es
        pop     ds              ; ds:si = the boot program's table
        mov     es, ax
        mov     di, _bootinfo
        mov     cx, BI_SIZE / 2
        rep movsw
        mov     ds, ax

        int     0x12            ; the BIOS is not there to ask later
        mov     [_basemem], ax
        mov     ah, 0x88
        int     0x15
        jnc     .ext
        xor     ax, ax
.ext:   mov     [_extmem], ax
        call    rdload
        mov     dx, 0x3F2       ; stop the diskette drive's motor: the
        mov     al, 0x0C        ;   BIOS will not be there to
        out     dx, al

        cli
        mov     ax, ds
        mov     ss, ax
        mov     sp, _uslot + UBYTES

; Address line 20.  Held low, as the BIOS leaves it, memory above 1M
; cannot be reached.  A PS/2 switches it with bit 1 of port 92h; an AT with
; the keyboard controller's output port.
        call    a20
        jne     .a20on
        in      al, 0x92
        or      al, 2
        and     al, 0xFE        ; bit 0 would reset the processor
        out     0x92, al
        call    a20
        jne     .a20on
        call    kbwait
        mov     al, 0xD1        ; write output port
        out     0x64, al
        call    kbwait
        mov     al, 0xDF
        out     0x60, al
        call    kbwait
        call    a20
        jne     .a20on
        mov     si, noa20
.say:   mov     al, [cs:si]
        inc     si
        or      al, al
        jz      .stop
        mov     ah, 0x0E
        xor     bx, bx
        int     0x10
        jmp     .say
.stop:  hlt
        jmp     .stop
.a20on:

; A descriptor for each segment the boot program loaded.
        mov     si, _bootinfo + BI_SEG
        mov     di, _gdt + GDT_KERNEL * 8
        mov     cx, [_bootinfo + BI_NSEG]
.seg:   mov     ax, [si]        ; where it is, as a real-mode segment
        mov     dx, ax
        shl     ax, 4
        shr     dx, 12          ; dx:ax = its physical address
        mov     bx, [si+4]
        mov     [di], bx        ; limit
        mov     [di+2], ax
        mov     [di+4], dl
        mov     al, SD_DATA
        cmp     word [si+6], 1
        jne     .type
        mov     al, SD_CODE
.type:  mov     [di+5], al
        add     si, 8
        add     di, 8
        loop    .seg

; The tables' own addresses, and the task state segment.
        mov     ax, ds
        mov     cx, ax
        shl     ax, 4
        shr     cx, 12
        mov     bx, ax          ; cx:bx = physical address of the data segment
        mov     dx, cx
        add     ax, _gdt
        adc     dx, 0
        mov     word [gdtptr], NGDT * 8 - 1
        mov     [gdtptr+2], ax
        mov     [gdtptr+4], dx
        mov     ax, bx
        mov     dx, cx
        add     ax, _idt
        adc     dx, 0
        mov     word [idtptr], NIDT * 8 - 1
        mov     [idtptr+2], ax
        mov     [idtptr+4], dx
        add     bx, _ktss
        adc     cx, 0
        mov     di, _gdt + GDT_TSS * 8
        mov     word [di], TS_SIZE - 1
        mov     [di+2], bx
        mov     [di+4], cl
        mov     byte [di+5], SD_TSS

; The BIOS has the first interrupt controller at vectors 8 to 15, which
; are exceptions on a 286.  Start both again at 32 and 40, with every
; request masked.
%macro  PIC 2
        mov     al, %2
        out     %1, al
        jmp     short $+2       ; give the controller time
%endmacro
        PIC     0x20, 0x11      ; edge triggered, cascaded, ICW4 follows
        PIC     0x21, T_IRQ0    ; vectors
        PIC     0x21, 0x04      ; the second controller is on request 2
        PIC     0x21, 0x01      ; 8086 mode
        PIC     0x21, 0xFF      ; mask
        PIC     0xA0, 0x11
        PIC     0xA1, T_IRQ0 + 8
        PIC     0xA1, 0x02
        PIC     0xA1, 0x01
        PIC     0xA1, 0xFF

        lgdt    [gdtptr]
        lidt    [idtptr]
        smsw    ax
        or      al, 3           ; protected mode; there is a 287
        lmsw    ax
        jmp     far pmode       ; empties the prefetch queue and loads CS

pmode:  mov     ax, DGROUP
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, _uslot + UBYTES - TF_SIZE   ; below a process's frame
        mov     [_ktss + TS_SS0], ax
        mov     word [_ktss + TS_SP0], _uslot + UBYTES
        mov     word [_uptr], _uslot
        xor     ax, ax
        lldt    ax
        mov     ax, GDT_TSS << 3
        ltr     ax
        fninit
        fsetpm                  ; the 287 has a mode of its own
        call    far _main

; main returns when there is a first process to run.  It has filled in
; the frame at the top of the kernel stack, as if that process had been
; interrupted; resume it.  The stack pointer is at that frame again.
        cli
        jmp     trapret

noa20:  db      "unix: cannot enable address line 20", 13, 10, 0

; Icode is what the first process runs: it executes /etc/init, and exits
; if it cannot.  icodestart() (machdep.c) copies it to a code segment of
; that process, at offset 0, so everything in it is addressed relative
; to _icode; the lists of arguments are near pointers into that segment.
; To the kernel it is data.
        global  _icode, _szicode, _initflags
        DATA
_icode: mov     ax, 59          ; execve(init, argv, 0)
        mov     bx, init - _icode
        mov     cx, cs
        mov     dx, argv - _icode
        mov     si, cs
        xor     di, di
        xor     bp, bp
        int     0x30
        mov     ax, 1           ; exit
        int     0x30
.hang:  jmp     .hang
init:   db      "/etc/init", 0
_initflags:
        db      "-00000", 0     ; decimal ASCII initflags
        align   2
argv:   dw      init + 5 - _icode
        dw      _initflags - _icode
        dw      0
icodeend:
_szicode:
        dw      icodeend - _icode

        TEXT

; The RAM disk image, if the boot program's table names one: read it from
; the drive booted from, a track at a time, through a buffer at RDBUF,
; and have the BIOS move each piece above 1M, which nothing else can do
; from real mode.  If anything fails there is no RAM disk.
RDBUF   equ     0x8000          ; the buffer, as a real-mode segment

rdload: cmp     word [_bootinfo + BI_RDSIZE], 0
        je      .done
        cmp     word [_bootinfo + BI_RDSECT], 0 ; it is there already
        je      .done
        cmp     word [_extmem], 0
        je      .fail
        mov     dl, [_bootinfo + BI_DRIVE]
        mov     ah, 8           ; the drive's shape
        push    es
        int     0x13
        pop     es
        jc      .fail
        and     cx, 0x3F
        mov     [rdspt], cx
        mov     cl, dh
        xor     ch, ch
        inc     cx
        mov     [rdheads], cx
        mov     si, [_bootinfo + BI_RDSECT]     ; the next sector to read
        xor     di, di                          ; sectors done
.next:  mov     bp, [_bootinfo + BI_RDSIZE]
        sub     bp, di          ; sectors to go
        jz      .done
        mov     ax, si
        xor     dx, dx
        div     word [rdspt]    ; dx = sector within its track, from 0
        mov     cx, dx
        mov     bx, [rdspt]
        sub     bx, dx          ; the rest of the track
        cmp     bx, bp
        jbe     .some
        mov     bx, bp
.some:  inc     cl
        xor     dx, dx
        div     word [rdheads]  ; ax = cylinder, dx = head
        mov     ch, al
        shl     ah, 6
        or      cl, ah
        mov     dh, dl
        mov     dl, [_bootinfo + BI_DRIVE]
        mov     bp, 3           ; tries
.read:  push    bx
        push    es
        mov     al, bl
        mov     ah, 2
        mov     bx, RDBUF
        mov     es, bx
        xor     bx, bx
        int     0x13
        pop     es
        pop     bx
        jnc     .move
        xor     ax, ax
        int     0x13
        dec     bp
        jnz     .read
        jmp     short .fail
.move:  mov     word [mvgdt+16], 0xFFFF         ; from the buffer
        mov     word [mvgdt+18], (RDBUF << 4) & 0xFFFF
        mov     byte [mvgdt+20], RDBUF >> 12
        mov     byte [mvgdt+21], SD_DATA
        mov     word [mvgdt+24], 0xFFFF         ; to 1M + 512 * di
        mov     ax, di
        mov     dx, di
        shl     ax, 9
        shr     dx, 7
        add     dl, 0x10
        mov     [mvgdt+26], ax
        mov     [mvgdt+28], dl
        mov     byte [mvgdt+29], SD_DATA
        mov     cx, bx
        shl     cx, 8           ; words
        push    si
        mov     si, mvgdt
        mov     ah, 0x87
        int     0x15
        pop     si
        jc      .fail
        add     si, bx
        add     di, bx
        jmp     .next
.fail:  mov     word [_bootinfo + BI_RDSIZE], 0
.done:  ret

; Is address line 20 working?  Returns "not equal" if it is.  With it held
; low, FFFF:7E0E is the same memory as 0000:7DFE, the end of the boot
; sector.  With it working that word is the RAM disk's, which rdload has
; filled by now, so what was there is put back whichever it is.
a20:    push    ds
        push    es
        xor     ax, ax
        mov     ds, ax
        not     ax
        mov     es, ax
        mov     ax, [0x7DFE]
        push    ax
        push    word [es:0x7E0E]
        not     ax
        mov     [es:0x7E0E], ax
        cmp     ax, [0x7DFE]
        pop     word [es:0x7E0E]        ; (the flags stay as they are)
        pop     ax
        mov     [0x7DFE], ax
        pop     es
        pop     ds
        ret

; Wait for the keyboard controller to take what it was sent.
kbwait: xor     cx, cx
.wait:  in      al, 0x64
        test    al, 2
        loopnz  .wait
        ret
