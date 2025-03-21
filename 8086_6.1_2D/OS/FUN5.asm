cpu 8086

%define $VGAx 320
%define $VGAy 200

org 4

IP:   dw Entry
Type: db 1

db "FUN5   ."

Entry:

hlt

mov DI, [Py]
mov CX, DI
shl DI, 1
shl DI, 1
add DI, CX
mov CL, 6
shl DI, CL
add DI, [Px]   ; DI = Py * $VGAx + Px

mov DL, [Vx]   ; DL: n
mov AL, DL     ; AL: Vx * 2
mov AH, [Vy]   ; AH: Vy * 2
shl AX, 1

mov SI, 0xA000
mov DS, SI
mov SI, $VGAx

mov CX, 0x70   ; Amount
mov DH, 0x0F   ; Color

Loop0:
   mov [DI], DH
   jcxz End
   inc DI

   add DL, AH
   cmp DL, AL
   jb Loop0
   sub DL, AL
   add DI, SI
   jmp Loop0
End:

hlt

push CS
pop DS

mov BL, 1
int 0

Px: dw ($VGAx >> 1)
Py: dw ($VGAy >> 1)
Vx: db 7
Vy: db 2


times 512-($-$$) db 0