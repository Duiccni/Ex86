cpu 8086

xor ax, ax										; clear AX
mov ds, ax										; set data segment


mov word [0x10 * 4], ax						; set INT 10h
mov word [0x10 * 4 + 2], 0xF010

mov word [0x13 * 4], ax						; set INT 13h
mov word [0x13 * 4 + 2], 0xF020

mov word [0x16 * 4], ax						; set INT 16h
mov word [0x16 * 4 + 2], 0xF030


not ah      									; set stack
mov ss, ax
mov sp, 0x1000


mov ax, 0x0201
xor cx, cx
xor dx, dx
mov bx, 0x7C00

int 13h


mov dx, 100h
mov ah, 2

int 10h


mov ax, 0xF000
mov ds, ax
mov si, msg0

mov ax, 0xB800
mov es, ax
xor di, di

mov cx, msg0end - msg0
print0:
   movsb
   inc di
   loop print0

wait0:
   in al, 0 ; int 16h
   cmp al, 20h
   jne wait0


jmp 0000:0x7C00

msg0: db "Bios transfering control to bootloader. Press space to continue. . ."
msg0end: