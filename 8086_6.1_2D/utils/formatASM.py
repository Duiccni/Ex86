a = list("""cpu 8086

org 0x7C00

; %define $VGAx 0x50
%define $pProcTable 0x400
%define $pDriveTable 0x600
%define $sINIT1 0x80
%define $pINIT1 sINIT1 << 4
%define $iENTRY 12


; Set the Stack
xor ax, ax
mov es, ax
mov ss, ax
mov ds, ax
mov sp, 0x7C00

call Print_Progress


; Clear Process table
; ASSUME: ES = 0
cld
xor ax, ax
mov di, $pProcTable
mov cx, 0x100
rep stosw


; Reset Disk System
; ASSUME: AX = 0
mov dx, ax
int 13h
jc Error


; Load Drive Apps table
; ASSUME: ES = CX = DX = 0
mov ax, 0x201
mov cl, 2
mov bx, $pDriveTable
int 13h
jc Error

call Print_Progress


; Check and Get for INIT1 app
mov ax, [$pDriveTable]

; Get ID to DL
mov dl, ah
mov cl, 4
shr dl, cl
test dl, dl
jnz Error      ; Error if ID != 0

; Load INIT1 ram segment into Proc table
mov word [$pProcTable], $sINIT1

call ToCHS                    ; Calculate first sector
mov al, [$pDriveTable + 2]    ; Get size

; Load INIT1 into 80:0
; ASSUME: ES = DL = 0
mov ah, 2
mov bx, $pINIT1
int 13h
jc Error

; Set ID of INIT1 in App Ram
; ASSUME: BX = $pINIT1, DL = 0
mov [bx], dl

call Print_Progress

jmp $sINIT1:$iENTRY


Error:
   mov ax, (0Eh << 8) | '*'
   xor bx, bx
   int 10h
End:
   hlt
   jmp End


; IN:
;  AX's first 12 bit: Sector [0, 2880)
; OUT:
;  CH: Cylinder [0, 80)
;  DH: Head (0 or 1)
;  CL: Sector [1, 18]
; AX = (DH + CH * 2) * 18 + CL
ToCHS:
   and ax, 0xFFF
   xor dh, dh
   mov cl, 36
   div cl
   mov cl, ah
   sub ah, 18
   jb Head0

   mov dh, 1
   mov cl, ah
Head0:
   inc cx
   mov ch, al
   ret


Print_Progress:
   mov ax, 0xE30
   inc byte [$-2]
   xor bx, bx
   int 10h
   ret

; Bootloader Signature
times 510-($-$$) db 0
dw 0xAA55""")

REGS = ["al", "bl", "cl", "dl", "ah", "bh", "ch", "dh",
        "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
        "cs", "ds", "ss", "es"]

i = 0
while i < len(a):
   if ''.join(a[i:i+2]).lower() in REGS:
      a[i] = a[i].upper()
      a[i+1] = a[i+1].upper()
      i += 1
   i += 1

print(''.join(a))