cpu 8086

; 0x00h implemented

test ah, ah
jz Read_key_press

iret


Read_key_press:
   in al, 0
   iret