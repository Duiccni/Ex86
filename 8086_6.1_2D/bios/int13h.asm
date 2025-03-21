cpu 8086

; 02h implemented

cmp ah, 15h
je getdrivetype

cmp ah, 2
jne end

load_sectors:
   mov ah, al
   in al, 1
   test al, al
   mov al, ah
   stc
   jnz err_end
end:
   clc
err_end:
   xor ah, ah
   retf 2

getdrivetype:
   mov AL, 1
   iret