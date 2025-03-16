cpu 8086

loop0:
   hlt
   jmp loop0

mov ax, 0x100
mov ds, ax
mov es, ax

mov ax, 0x0204
mov cx, 2
xor dx, dx
mov bx, 0x100

int 13h

jmp 0x100:0x100

times 510-($-$$) db 0
dw 0xAA55