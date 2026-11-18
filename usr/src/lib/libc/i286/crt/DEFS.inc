; Assembly conventions for the 286 run-time support routines (NASM).
;
; Assemble with -DFAR for the medium and large models, where compiled code
; reaches these with a far call.

        bits    16
        cpu     286

%ifdef FAR
  %define RET   retf
%else
  %define RET   ret
%endif

%macro ENTRY 1
        global  %1
%1:
%endmacro

        segment _TEXT public align=2 class=CODE
