; Conventions and constants for the kernel's assembly (NASM).  seg.h,
; frame.h and trap.h say the same to the compiler; change both.
;
; The kernel is medium model: C functions are called far and return far,
; and DS and SS are always the kernel's one data segment.

        bits    16
        cpu     286

GDT_TSS         equ     1
GDT_LDT         equ     2
GDT_KERNEL      equ     16
NKSEG           equ     16
NGDT            equ     GDT_KERNEL + NKSEG

SD_CODE         equ     0x9B
SD_DATA         equ     0x93
SD_TSS          equ     0x81
SD_INTGATE      equ     0x86    ; gate that turns interrupts off
SD_TRAPGATE     equ     0x87    ; gate that leaves them as they were
SD_USER         equ     0x60    ; added to a gate: a process may use it

T_BPT           equ     3
T_OFLOW         equ     4
T_IRQ0          equ     32      ; the 16 interrupt requests
T_SYSCALL       equ     48
NIDT            equ     49

TF_SIZE         equ     32      ; struct trapframe (frame.h)

EFAULT          equ     14      ; h/errno.h

; struct bootinfo (boot.h)
BI_NSEG         equ     2
BI_DSEG         equ     8
BI_SEG          equ     12
BI_SIZE         equ     12 + 16 * 8

; struct tss (seg.h)
TS_SP0          equ     2
TS_SS0          equ     4
TS_SIZE         equ     44

; All of the kernel's assembly shares one code segment, so its routines
; reach each other with near calls.  TEXT, DATA and BSS choose where what
; follows goes.
        segment MCH_TEXT public align=2 class=CODE
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _DATA _BSS

%define TEXT    segment MCH_TEXT
%define DATA    segment _DATA
%define BSS     segment _BSS

; A routine C can call: far, with the name C knows it by.
%macro  ENTRY 1
        global  _%1
_%1:
%endmacro
