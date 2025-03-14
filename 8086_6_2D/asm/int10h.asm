cpu 8086

; 0x0Eh implemented

push ds
push cs
pop ds

cmp ah, 0Eh
je teletype_output
cmp ah, 02h
je set_cursor_pos

pop ds
iret


set_cursor_pos:
   ; mul DH by 80 = 5 * 16 = (4 + 1) * 16
   mov al, dh
   shl al, 1
   shl al, 1
   add al, dh     ; al = dh * 5

   cbw            ; xor ah, ah
   mov cl, 4
   shl ax, cl     ; ax = dh * 80
   
   xor dh, dh
   add ax, dx     ; ax = dh * 80 + dl

   mov [text_index], ax

   pop ds
   iret


teletype_output:
   mov bx, [text_index]
   shl bx, 1
   inc word [text_index]

   mov dx, 0xB800
   mov ds, dx
   mov [bx], al

   pop ds
   iret


text_index: dw 0
