cpu 8086

; 0x0Eh implemented

push ds

cmp ah, 0Eh
je teletype_output
cmp ah, 0Ch
je set_pixel
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

   mov cs:[text_index], ax

   pop ds
   iret


; (CX, DX) = (x, y), AL = COLOR
set_pixel:
   mov bx, 0xA000
   mov ds, bx

   mov bx, dx
   shl dx, 1
   shl dx, 1
   add dx, bx     ; dx = dx * 5

   mov bx, cx

   mov cl, 6
   shl dx, cl

   add bx, dx

   mov [bx], al

   pop ds
   iret


teletype_output:
   mov bx, 0xB800
   mov ds, bx

   mov bx, cs:[text_index]
   shl bx, 1
   inc word cs:[text_index]

   mov [bx], al

   pop ds
   iret


text_index: dw 0
