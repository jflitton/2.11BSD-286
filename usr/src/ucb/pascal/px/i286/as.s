; px, the Berkeley Pascal interpreter, for the 286: what the PDP-11's
; Makefile assembles at once, E.s and the files of the interpreter's
; operations, here in NASM's syntax, which the system's as reads
; (docs/toolchain.md).  Each file here is the PDP-11's of the same name
; written again for the 286, routine for routine and in the same order;
; the PDP-11's are in the directory above.
;
; What the PDP-11's registers were:
;
;	lc (r4)		SI, the interpreter's location counter, in the
;			data segment, where int.c reads the code
;	lp (r5)		the loop: NEXT is the PDP-11's return, jmp (lp)
;	r3		on entry to an operation, CX: its subop, the byte
;			after it, extended, with ZF set when it is 0, as
;			the PDP-11's condition codes were; and BX is twice
;			the operation, a signed byte, as r0 was
;	r0 to r2	AX, BX, DX, DI and BP, as each routine has them
;	fr0 to fr3	the 287's stack, which is empty again after each
;			operation
;
; The interpreter's stack is the machine's, as on the PDP-11, with DS
; and SS the same.  What is on it is as it is in memory: a long's low
; word is on top, where the PDP-11 has its high word, and a real is the
; 287's double.  The 287 rounds to the nearest, and __CHP (the C
; library's) truncates where the PDP-11's FPU did, in converting a real
; to an integer.  A real that comes out of an operation too large for
; a double is the error the PDP-11's overflow was (fput, 06add.s).
;
; What px calls of C keeps SI, DI and BP, and may change ES, which a
; string instruction here loads first; the direction flag is clear.
; The routines of the I/O here (30iosubs.s) keep every register but AX.
;
; The PDP-11's errors were an emulator trap, whose handler called
; error() (perror.c) with the number in _perrno; here ERROR jumps there.
; Its constants are made of Edata, as E.s is on the PDP-11, by
; i286/Emake.ed, in E.inc.  The numbers here are decimal or hexadecimal,
; which the assembler reads, where the PDP-11's are octal.
;
; 30atof.s, the PDP-11's atof, is not here: the C library's is called.
; What C px calls by names the C library has too, which ctime brings
; with its stdio, are named apart: v6flush is the V6 library's fflush
; (putc.s), and pfree palloc.c's free.

        bits    16
        cpu     286
        segment _TEXT public align=2 class=CODE
        segment _DATA public align=2 class=DATA
        segment _BSS public align=2 class=BSS
        group   DGROUP _DATA _BSS

%include "E.inc"

%macro  NEXT 0
        jmp     loop
%endmacro

%macro  ERROR 1
        mov     word [_perrno], %1
        jmp     onemt
%endmacro

HZ      equ     60              ; the clock's rate (-DHZ for the C)

; The file structure (30iosubs.s): these below the window, which a file
; variable points at, and the buffer of getc and putc, 518 bytes, below
; them.
FBUF    equ     -14
FCHAIN  equ     -12
FLEV    equ     -10
PFNAME  equ     -8
FNAME   equ     -6
FUNIT   equ     -4
FSIZE   equ     -2
FBUFSZ  equ     518             ; getc's buffer: descriptor, count,
                                ; next, and 512 bytes

; The flags in FUNIT, with the file's descriptor in the low 4 bits
EOF     equ     0x100
EOLN    equ     0x200
SYNC    equ     0x400
TEMP    equ     0x800
FREAD   equ     0x1000
FWRITE  equ     0x2000
FTEXT   equ     0x4000

; Operations that are given to pwrite() (pwrite.c), as opcode.h has them
O_WRIT2 equ     0xD0
O_WRIT4 equ     0xD1
O_WRITB equ     0xD2
O_WRITC equ     0xD3
O_WRIT8 equ     0xD4
O_WRITG equ     0xD5
O_WRIT82 equ    0xD6
O_WRITLN equ    0xD7
O_WROCT2 equ    0xD8
O_WRHEX2 equ    0xBE

        extern  _alloc, _pfree, _error, _pcttot, _pdattim, _pwrite
        extern  _psexit, _abort
        extern  _atan, _cos, _exp, _log, _sin, _sqrt, _atof
        extern  _creat, _open, _close, _unlink, _write, _lseek, _time
        extern  _times, _perror, __exit, __CHP
        extern  _v6flush, _getc, _putc, _errno
        extern  _display, _dp, _lino, _nodump, _file, _maxstk
        extern  _argc, _argv, _seed, _randa, _randc, _randm, _randim

%include "i286/00int.s"
%include "i286/02rel.s"
%include "i286/02relset.s"
%include "i286/03bool.s"
%include "i286/04as.s"
%include "i286/05lv.s"
%include "i286/06add.s"
%include "i286/07sub.s"
%include "i286/10mul.s"
%include "i286/12div.s"
%include "i286/13mod.s"
%include "i286/14neg.s"
%include "i286/16dvd.s"
%include "i286/17ind.s"
%include "i286/17rv.s"
%include "i286/20con.s"
%include "i286/21rang.s"
%include "i286/24case.s"
%include "i286/24pxp.s"
%include "i286/25set.s"
%include "i286/26for.s"
%include "i286/27conv.s"
%include "i286/30getname.s"
%include "i286/30io.s"
%include "i286/30iosubs.s"
%include "i286/30read.s"
%include "i286/30write.s"
%include "i286/34fun.s"
%include "i286/opcode.s"
