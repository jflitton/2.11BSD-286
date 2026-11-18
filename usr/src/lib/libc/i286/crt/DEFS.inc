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
