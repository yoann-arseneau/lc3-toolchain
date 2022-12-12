.org x3000
LEA r0, hello  ; puts("Hello World!\n")
OUT            ; -
HALT           ; abort();
AND r5, r2, r5
ADD r0, r1, #5
ADD r2,r3,#-16
AND r4, r5, #+1
hello .stringz "Hello World!\n"

