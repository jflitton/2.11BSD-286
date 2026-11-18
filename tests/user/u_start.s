; Start of the process in tests/user (NASM): small model, ring 3.  The
; kernel has set CS, DS, ES, SS and the stack pointer.  Also the system
; calls, and the instructions that are meant to fault.

        bits    16
        cpu     286

        segment _TEXT public align=2 class=CODE
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _DATA _BSS

        extern  _main
        global  _errno, _lasterr

        segment _BSS
_errno:   resw  1
_lasterr: resw  1               ; error code of the last fault
savesp:   resw  1
farptr:   resw  2
zero:     resw  1

        segment _TEXT
..start:
        call    _main
        push    ax
        call    _exit
.hang:  jmp     .hang

%macro  ENTRY 1
        global  _%1
_%1:
%endmacro

; System calls: the number in AX, the arguments a word each in BX, CX,
; DX, SI, DI and BP, INT 30h.  A pointer is two words, offset and then
; selector, whatever the program's memory model; here that is DS.  The
; kernel returns with carry clear and the value in AX, or DX:AX; or with
; carry set and the error number in AX.  These are what libc's stubs will
; be; SI, DI and BP are the C caller's and are put back.
%macro  SYS 1
        mov     ax, %1
        int     0x30
%endmacro

cerror: mov     [_errno], ax
        mov     ax, -1
        cwd
        ret

; exit(code)
ENTRY exit
        mov     bx, sp
        mov     bx, [bx+2]
        SYS     0

; write(buf, n)
ENTRY write
        mov     bx, sp
        mov     dx, [bx+4]
        mov     bx, [bx+2]
        mov     cx, ds
        SYS     1
        jc      cerror
        ret

; add(a, b)
ENTRY add
        mov     bx, sp
        mov     cx, [bx+4]
        mov     bx, [bx+2]
        SYS     2
        jc      cerror
        ret

; ladd(a, b), longs
ENTRY ladd
        push    si
        mov     bx, sp
        mov     si, [bx+10]
        mov     dx, [bx+8]
        mov     cx, [bx+6]
        mov     bx, [bx+4]
        SYS     3
        pop     si
        jc      cerror
        ret

; fill(buf, n, c)
ENTRY fill
        push    si
        mov     bx, sp
        mov     si, [bx+8]
        mov     dx, [bx+6]
        mov     bx, [bx+4]
        mov     cx, ds
        SYS     4
        pop     si
        jc      cerror
        ret

; uticks()
ENTRY uticks
        SYS     5
        jc      cerror
        ret

; readfar(offset, selector, n), fillfar(offset, selector, n): a pointer
; given as its two words, so that the test can pass any selector.
ENTRY readfar
        mov     bx, sp
        mov     dx, [bx+6]
        mov     cx, [bx+4]
        mov     bx, [bx+2]
        SYS     6
        jc      cerror
        ret

ENTRY fillfar
        mov     bx, sp
        mov     dx, [bx+6]
        mov     cx, [bx+4]
        mov     bx, [bx+2]
        SYS     7
        jc      cerror
        ret

; sum6(a, b, c, d, e, f): every argument register.
ENTRY sum6
        push    si
        push    di
        push    bp
        mov     bx, sp
        mov     bp, [bx+18]
        mov     di, [bx+16]
        mov     si, [bx+14]
        mov     dx, [bx+12]
        mov     cx, [bx+10]
        mov     bx, [bx+8]
        SYS     8
        pop     bp
        pop     di
        pop     si
        jc      cerror
        ret

; sum8(a, b, c, d, e, f, g, h): more words than registers, so the call
; takes one argument, a pointer to them.  They are on the stack already.
ENTRY sum8
        mov     bx, sp
        add     bx, 2
        mov     cx, ss
        SYS     9
        jc      cerror
        ret

; sum8at(offset, selector): the same with the pointer given.
ENTRY sum8at
        mov     bx, sp
        mov     cx, [bx+4]
        mov     bx, [bx+2]
        SYS     9
        jc      cerror
        ret

; nosuch()
ENTRY nosuch
        SYS     99
        jc      cerror
        ret

; kept(): do registers that are not the result's survive a system call?
; Returns 1 if they do.
ENTRY kept
        push    si
        push    di
        push    bp
        mov     bx, 0x1111
        mov     cx, 0x2222
        mov     si, 0x3333
        mov     di, 0x4444
        mov     bp, 0x5555
        SYS     5
        xor     ax, ax
        cmp     bx, 0x1111
        jne     .no
        cmp     cx, 0x2222
        jne     .no
        cmp     si, 0x3333
        jne     .no
        cmp     di, 0x4444
        jne     .no
        cmp     bp, 0x5555
        jne     .no
        inc     ax
.no:    pop     bp
        pop     di
        pop     si
        ret

ENTRY getcs
        mov     ax, cs
        ret
ENTRY getds
        mov     ax, ds
        ret
ENTRY getss
        mov     ax, ss
        ret
ENTRY getflags
        pushf
        pop     ax
        ret

; setflags(value): try to load the flags; returns what they are afterwards.
ENTRY setflags
        mov     bx, sp
        push    word [bx+2]
        popf
        pushf
        pop     ax
        ret

; spin(): 65,536 times round a loop, about a clock tick's worth.
ENTRY spin
        xor     cx, cx
.loop:  loop    .loop
        ret

; What follows faults on purpose.  The kernel half of the test sends a
; fault from the process that finds 0xFEED in CX on to the address in BX,
; with the vector in AX and the error code in DX.  Each of these returns
; the vector, or -1 if there was no fault, and leaves the error code in
; lasterr.
%macro  FAULT 0
        mov     bx, .on
        mov     cx, 0xFEED
%endmacro
%macro  ENDFAULT 0
        mov     ax, -1
.on:    mov     [_lasterr], dx
        ret
%endmacro

ENTRY u_cli
        FAULT
        cli
        ENDFAULT

ENTRY u_hlt
        FAULT
        hlt
        ENDFAULT

ENTRY u_inb
        mov     dx, 0x21
        FAULT
        in      al, dx
        ENDFAULT

ENTRY u_outb
        mov     dx, 0x21
        mov     al, 0xFF
        FAULT
        out     dx, al
        ENDFAULT

ENTRY u_lmsw
        xor     ax, ax
        FAULT
        lmsw    ax
        ENDFAULT

; u_loades(selector)
ENTRY u_loades
        mov     bx, sp
        mov     ax, [bx+2]
        FAULT
        mov     es, ax
        push    ds
        pop     es
        ENDFAULT

; u_int20(): the clock's vector, whose gate is the kernel's alone.
ENTRY u_int20
        FAULT
        int     0x20
        ENDFAULT

; u_int31(): a vector past the end of the table.
ENTRY u_int31
        FAULT
        int     0x31
        ENDFAULT

; u_farcall(selector): far call to selector:0.
ENTRY u_farcall
        mov     bx, sp
        mov     ax, [bx+2]
        mov     word [farptr], 0
        mov     [farptr+2], ax
        FAULT
        call    far [farptr]
        ENDFAULT

; u_peek(offset): read a word of the data segment.
ENTRY u_peek
        mov     bx, sp
        push    si
        mov     si, [bx+2]
        FAULT
        mov     ax, [si]
        mov     ax, -1
.on:    pop     si
        mov     [_lasterr], dx
        ret

; u_pokecs(): write a word of the code segment.
ENTRY u_pokecs
        FAULT
        mov     [cs:0], ax
        ENDFAULT

ENTRY u_div0
        mov     ax, 1
        xor     dx, dx
        FAULT
        div     word [zero]
        ENDFAULT

ENTRY u_bpt
        FAULT
        int3
        ENDFAULT

ENTRY u_op386
        FAULT
        db      0x0F, 0xB6, 0xC3        ; MOVZX AX, BL
        ENDFAULT

; u_stack(): push with the stack pointer at 0, which wraps to the top of
; a segment that does not reach that far.
ENTRY u_stack
        mov     [savesp], sp
        FAULT
        xor     sp, sp
        push    ax
        mov     ax, -1
.on:    mov     sp, [savesp]
        mov     [_lasterr], dx
        ret
