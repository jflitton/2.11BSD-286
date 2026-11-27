; The interpreter's table of operations, made from the PDP-11's
; opcode.s, in its order: the operations 0200 to 0377 before optab,
; which the loop reaches by the operation as a signed byte, twice
; (00int.s), and 0 to 0177 after it.  O_WRITC, which the PDP-11's has
; here, is with the others in as.s.  The table is in the code, as the
; compiler keeps its tables of jumps.

        segment _TEXT
        dw      _CON1, _CON2, _CON4, _CON8
        dw      _RV, _IND, _CON, badop
        dw      _RANG2, _RANG42, _RSNG2, _RSNG42
        dw      _RANG4, _RANG24, _RSNG4, _RSNG24
        dw      badop, badop, badop, badop
        dw      badop, badop, badop, badop
        dw      badop, badop, badop, badop
        dw      badop, badop, badop, badop
        dw      _CASE1OP, _CASE2OP, _CASE4OP, _PXPBUF
        dw      _COUNT, badop, badop, badop
        dw      _ADDT, _SUBT, _MULT, _INCT
        dw      _CTTOT, _CARD, _IN, _ASRT
        dw      _FOR1U, _FOR2U, _FOR4U, _FOR1D
        dw      _FOR2D, _FOR4D, _STLIM, _SCLCK
        dw      _STOI, _STOD, _ITOD, _ITOS
        dw      _BUFF, _WCLCK, _WRHEX2, _WRHEX4
        dw      _GET, _PUT, _MESSAGE, _FNIL
        dw      _EOF, _EOLN, _RESET, _REWRITE
        dw      _REMOVE, _READ4, _UNIT, _READC
        dw      _READ8, _UNITINP, _UNITOUT, _READLN
        dw      _WRIT2, _WRIT4, _WRITB, _WRITC
        dw      _WRIT8, _WRITG, _WRIT82, _WRITLN
        dw      _WROCT2, _WROCT4, _FLUSH, _PACK
        dw      _UNPACK, _LLIMIT, _ARGC, _ARGV
        dw      _CLCK, _SEED, _RANDOM, _DISPOSE
        dw      _NEW, _EXPO, _DATE, _TIME
        dw      _ATAN, _COS, _EXP, _LN
        dw      _SIN, _SQRT, _CHR2, _CHR4
        dw      _ODD2, _ODD4, _PRED2, _PRED4
        dw      _PRED24, _SUCC2, _SUCC4, _SUCC24
        dw      _DEFNAME, _PAGE, _UNDEF, _SQR2
        dw      _SQR4, _SQR8, _ROUND, _TRUNC
optab:
        dw      _ABORT, _HALT, badop, _NODUMP
        dw      _BEG, _END, _CALL, _TRACNT
        dw      _PUSH, _POP, _INX4, _SDUP
        dw      _IF, _TRA, _LINO, _GOTO
        dw      _REL2, _REL4, _REL24, _REL42
        dw      _REL8, _RELG, _RELT, badop
        dw      _REL28, _REL48, _REL82, _REL84
        dw      _AND, _OR, _NOT, badop
        dw      _AS2, _AS4, _AS24, _AS42
        dw      _AS8, _INX2P2, _INX4P2, _AS
        dw      _AS21, _AS41, _AS28, _AS48
        dw      _OFF, _INX2, _NIL, _LV
        dw      _ADD2, _ADD4, _ADD24, _ADD42
        dw      _ADD28, _ADD48, _ADD82, _ADD84
        dw      _SUB2, _SUB4, _SUB24, _SUB42
        dw      _SUB28, _SUB48, _SUB82, _SUB84
        dw      _MUL2, _MUL4, _MUL24, _MUL42
        dw      _MUL28, _MUL48, _MUL82, _MUL84
        dw      _ABS2, _ABS4, _ABS8, badop
        dw      _ADD8, _SUB8, _MUL8, _DVD8
        dw      _DIV2, _DIV4, _DIV24, _DIV42
        dw      badop, badop, badop, badop
        dw      _MOD2, _MOD4, _MOD24, _MOD42
        dw      badop, badop, badop, badop
        dw      _NEG2, _NEG4, _NEG8, badop
        dw      badop, badop, badop, badop
        dw      badop, badop, badop, badop
        dw      badop, badop, badop, badop
        dw      _DVD2, _DVD4, _DVD24, _DVD42
        dw      _DVD28, _DVD48, _DVD82, _DVD84
        dw      _RV1, _RV2, _RV4, _RV8
        dw      _IND1, _IND2, _IND4, _IND8

