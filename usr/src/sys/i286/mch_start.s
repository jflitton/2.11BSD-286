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
; enters protected mode and calls main.  Everything before the mode switch
; uses near jumps and calls only: the far addresses in the kernel are
; protected-mode selectors, which mean nothing yet.

%include "mch_defs.inc"

        extern  _main
        extern  _idt:wrt DGROUP
        global  _bootinfo, _basemem, _extmem, _gdt, _ktss

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
        resb    2048            ; the stack until there are processes
bootstack:

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

        cli
        mov     ax, ds
        mov     ss, ax
        mov     sp, bootstack

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
        mov     sp, bootstack
        mov     [_ktss + TS_SS0], ax
        mov     word [_ktss + TS_SP0], bootstack
        xor     ax, ax
        lldt    ax
        mov     ax, GDT_TSS << 3
        ltr     ax
        fninit
        fsetpm                  ; the 287 has a mode of its own
        call    far _main
.halt:  hlt
        jmp     .halt

noa20:  db      "unix: cannot enable address line 20", 13, 10, 0

; Is address line 20 working?  Returns "not equal" if it is.  With it held
; low, FFFF:7E0E is the same memory as 0000:7DFE, the end of the boot
; sector.
a20:    push    ds
        push    es
        xor     ax, ax
        mov     ds, ax
        not     ax
        mov     es, ax
        mov     ax, [0x7DFE]
        push    ax
        not     ax
        mov     [es:0x7E0E], ax
        cmp     ax, [0x7DFE]
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
