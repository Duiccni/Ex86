cpu 8086

org 4

IP:   dw Entry
Type: db 1

db "FUN6   ."

Entry:

%define $VGAx 320
%define $VGAy 200

hlt

mov SI, 0xA000
mov SS, SI

mov SI, ($VGAx >> 1)
mov DI, ($VGAy >> 1)

L0:
   mov BP, DI
   mov CX, BP
   shl BP, 1
   shl BP, 1
   add BP, CX
   mov CL, 6
   shl BP, CL
   add BP, SI

   mov byte [BP], 0xFF

IX:inc SI
   cmp SI, $VGAx
   jnc lCX
IY:inc DI
   cmp DI, $VGAy
   jc L0
   xor byte [IY], 8
   jmp IY
lCX:
   xor byte [IX], 8
   jmp IX


times 512-($-$$) db 0