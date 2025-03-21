cpu 8086

cmp ah, 2Ch
je get_system_time
cmp ah, 09h
je print_string

iret


get_system_time:
   push ax
   in ax, 2
   mov dx, ax
   pop ax
   iret


print_string:
   mov ah, 0Eh
   mov si, dx
   cld
loop0:
   lodsb
   int 10h
   cmp al, '$'
   jne loop0

   iret