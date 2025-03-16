cpu 8086

xor ax, ax										; clear AX
mov ds, ax										; set data segment


mov word [0x12 * 4], int12h				; set INT 12h
mov word [0x12 * 4 + 2], 0xF000

mov word [0x10 * 4], ax						; set INT 10h
mov word [0x10 * 4 + 2], 0xF010

mov word [0x13 * 4], ax						; set INT 13h
mov word [0x13 * 4 + 2], 0xF020

mov word [0x16 * 4], ax						; set INT 16h
mov word [0x16 * 4 + 2], 0xF030

mov word [0x1E * 4], ax						; set Drive Description Table
mov word [0x1E * 4 + 2], 0xF034

mov word [0x21 * 4], ax						; set INT 21h
mov word [0x21 * 4 + 2], 0xF036



not ah      									; set stack
mov ss, ax
mov sp, 0x1000


mov ax, 0x0201
mov cx, 1
xor dx, dx
mov bx, 0x7C00

int 13h


mov dx, 100h
mov ah, 2

int 10h


cmp word [0x7C00 + 0x200 - 2], 0xAA55
jne no_bootable

mov si, msg0
mov cx, msg0end - msg0
jmp continue0

no_bootable:
mov si, msg1
mov cx, msg1end - msg1

continue0:


mov ax, 0xF000
mov ds, ax

mov ax, 0xB800
mov es, ax
xor di, di

print0:
   movsb
   inc di
   loop print0

hlt


cmp si, msg0end
jne end
jmp 0000:0x7C00

end:
   hlt
   jmp end

msg0: db "Jumping to bootloader, press a key."
msg0end:
msg1: db "No bootable device."
msg1end:

int12h:
   mov ax, 0x0280
   iret