; Assembly conventions for the C library and the run-time support routines
; on the 286 (NASM).
;
; Assemble with -DFAR for the medium and large models, where compiled code
; reaches every function with a far call.  Then:
;
;       RET     returns as the model's functions do: RETF, or RET
;       CALLC   calls a C function, or any other in the library: far or
;               near
;       RA      is the size of a return address, 4 or 2: on entry the
;               first argument is at [SP+RA], and after PUSH BP and
;               MOV BP, SP at [BP+2+RA]
;
; And with -DFARDATA besides for the large model, where a data pointer is
; far, four bytes, and compiled code keeps nothing in DS: it is loaded
; with the segment of whatever is pointed at, as ES is, and a function
; need not put it back.  The program's own data, which the stack is in,
; is reached through SS.  So in a file that is assembled for the large
; model, what is addressed through BX, SI or DI is in the segment DS was
; last given, and the stack and the program's variables are written with
; SS: or addressed through BP.

        bits    16
        cpu     286

%ifdef FAR
  %define RET   retf
  %define CALLC call far
RA      equ     4
%else
  %define RET   ret
  %define CALLC call
RA      equ     2
%endif

%macro ENTRY 1
        global  %1
%1:
%endmacro

        segment _TEXT public align=2 class=CODE
