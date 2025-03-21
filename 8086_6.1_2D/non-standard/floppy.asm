cpu 8086

xor ax, ax
mov es, ax

mov ax, 0x0201
mov cx, 0x0045
mov dx, 0x0100
mov bx, 0x100

int 13h

jmp 0000:0x100

; mov ax, 0x100
; mov ds, ax
; mov es, ax
; 
; mov ax, 0x0204
; mov cx, 2
; xor dx, dx
; mov bx, 0x100
; 
; int 13h
; 
; jmp 0x100:0x100

times 510-($-$$) db 0
dw 0xAA55

times 0x7400-($-$$) db 0

org 0x100-0x7400

xor ax, ax
mov ds, ax
mov ah, 0Eh
mov si, msg0
cld
  
loop0:
   lodsb
   int 10h
   test al, al
   jnz loop0

loop1:
   hlt
   jmp loop1

msg0: db "Hi I am OS loaded at 0000:0100 by bootloader.", 0
