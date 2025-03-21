cpu 8086

%define $VGAx 0x50
%define $sProcTable 0x40

org 4

IP:   dw Entry
Type: db 1

db "EXPLORER"

Entry:

hlt

mov SI, 0xB800
mov ES, SI

call PrintProcs

xor CH, CH
mov CL, 4
mov DI, L1

jmp L2

L1:
   mov AL, 'E'
   out 0, AL
   jc CallProc

   mov AH, CH

   mov AL, 'W'
   out 0, AL
   sbb CH, DL

   mov AL, 'S'
   out 0, AL
   adc CH, DL

   cmp AH, CH
   je L1
L2:
   mov SI, $VGAx * 4
   mov BL, CH
   shr BL, CL
   call putcH
   add SI, 2
   mov BL, CH
   and BL, 0xF
   push DI
   jmp putcH
CallProc:
   mov BL, CH

   mov SI, msg0
   mov DI, $VGAx * 2
   mov AX, CS
   mov DS, AX
   mov AH, ~0x8F
   mov CX, mgs0end - msg0
L3:
   lodsb
   stosw
   loop L3

mov AH, BL
mov BL, 4
int 0


PrintProcs:
   xor DX, DX
   jmp FS_L0
L0:
   inc DX
   test DL, DL
   jz L0_End
FS_L0:
   mov SI, $sProcTable
   mov DS, SI

   mov SI, DX
   shl SI, 1

   mov AX, [SI]
   test AX, AX
   jz L0

   shl SI, 1
   add SI, DX
   mov CL, 5
   shl SI, CL
   add SI, $VGAx * 6

   mov CL, 4

   mov BL, DL
   shr BL, CL
   call putcH
   add SI, 2
   mov BL, DL
   and BL, 0xF
   call putcH

   add SI, 2
   mov byte ES:[SI], ':'
   add SI, 4

   mov BL, AH
   shr BL, CL
   call putcH
   add SI, 2
   mov BL, AH
   and BL, 0xF
   call putcH
   add SI, 2
   mov BL, AL
   shr BL, CL
   call putcH
   add SI, 2
   mov BL, AL
   and BL, 0xF
   call putcH


   mov DI, SI
   add DI, 4

   hlt
   mov DS, AX
   mov SI, 7

   mov AH, ~0x8F
   mov CX, 8
L4:
   lodsb
   stosw
   loop L4

   jmp L0
L0_End:
   ret

putcH:
   cmp BL, 10
   jb decimal
   add BL, 'A' - 10
   jmp cont0
decimal:
   or BL, '0'
cont0:
   mov ES:[SI], BL
   ret

msg0: db "Calling the proccess. "
mgs0end:

times 512-($-$$) db 0