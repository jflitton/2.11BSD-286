; Conventions and constants for the kernel's assembly (NASM).  seg.h,
; frame.h and trap.h say the same to the compiler; change both.
;
; The kernel is medium model: C functions are called far and return far,
; and DS and SS are always the kernel's one data segment.

        bits    16
        cpu     286

GDT_TSS         equ     1
GDT_LDT         equ     2
GDT_TMP1        equ     3
GDT_TMP2        equ     4
GDT_KERNEL      equ     16
NKSEG           equ     16
NGDT            equ     64

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

; struct trapframe (frame.h)
TF_VEC          equ     18
TF_IP           equ     22
TF_CS           equ     24
TF_SIZE         equ     32

UBYTES          equ     7 * 512 ; machparam.h
INTSTACK        equ     3072    ; size of the stack interrupts run on

; Interrupt priority levels (machparam.h)
IPL_SOFTCLOCK   equ     1
IPL_NET         equ     2
IPL_BIO         equ     3
IPL_IMP         equ     4
IPL_TTY         equ     5
IPL_CLOCK       equ     6
IPL_HIGH        equ     7

ENOENT          equ     2       ; h/errno.h
EFAULT          equ     14

; struct bootinfo (boot.h)
BI_NSEG         equ     2
BI_DSEG         equ     8
BI_DRIVE        equ     10
BI_RDSECT       equ     12
BI_RDSIZE       equ     14
BI_SEG          equ     16
BI_SIZE         equ     16 + 16 * 8

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
