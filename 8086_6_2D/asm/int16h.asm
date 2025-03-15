cpu 8086

; 0x00h implemented

test ah, ah
jnz return

Read_key_press:
   in al, 0
return:
   iret