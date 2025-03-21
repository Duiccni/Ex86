cpu 8086

org 4

IP:   dw Entry
Type: db 1

db "FUN4   ."

Entry:

mov SI, 0xA000 + 0x142
mov DS, SI
xor SI, SI

mov CX, 0x30
L0:
   xor AL, AL
   L1:
      mov [SI], AL
      inc SI
      inc AL
      jnz L1
   add SI, 64
   loop L0

End:
   hlt
   jmp End

times 512-($-$$) db 0