cpu 8086

start:
   mov cx, 1999
   mov ah, 0x0E

   loop:
      mov al, cl
      and al, 0x0F
      add al, 0x41
      int 10h
      loop loop
   
   end:
      hlt
      jmp end
