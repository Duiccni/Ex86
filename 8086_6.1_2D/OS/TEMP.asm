cpu 8086

org 4

IP:   dw Entry
Type: db 1

Entry:

%define $VGAx 320
%define $VGAy 200

hlt

mov SI, 0xA000
mov SS, SI

mov SI, ($VGAx >> 1)
mov DI, ($VGAy >> 1)

xor DX, DX

SetPixel:
   mov BP, DI
   mov CX, BP
   shl BP, 1
   shl BP, 1
   add BP, CX
   mov CL, 6
   shl BP, CL
   add BP, SI

   mov byte [BP], 0xFF
   ret

times 512-($-$$) db 0