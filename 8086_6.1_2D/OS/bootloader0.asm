cpu 8086

org 0x7C00

; %define $VGAx 0x50
%define $pProcTable 0x400
%define $pDriveTable 0x600

%define $sINIT1 0xA0
%define $pINIT1 ($sINIT1 << 4)

%define $iAORG 4


; Set the Stack
xor AX, AX
mov ES, AX
mov SS, AX
mov DS, AX
mov SP, 0x7C00

call Print_Progress


; Clear Process table
; ASSUME: ES = 0
cld
xor AX, AX
mov DI, $pProcTable
mov CX, 0x100
rep stosw


; Reset Disk System
; ASSUME: AX = 0
mov DX, AX
int 13h
jc Error


; Load Drive Apps table
; ASSUME: ES = CX = DX = 0, DI = $pDriveTable
mov AX, 0x201
mov CL, 2
mov BX, DI
int 13h
jc Error

call Print_Progress


; Check and Get for INIT1 app
mov AX, [$pDriveTable]

; Get appID to DL
mov DL, AH
mov CL, 4
shr DL, CL
test DL, DL
jnz Error      ; Error if ID != 0

; Load INIT1 ram segment into Proc table, also setup ES for later
mov CX, $sINIT1
mov ES, CX
mov word [$pProcTable], CX

call ToCHS                    ; Calculate first sector
mov AL, [$pDriveTable + 2]    ; Get Drive Size

; Load INIT1 into A0:0
; ASSUME: ES = $sINIT1, DL = 0
mov AH, 2
mov BX, $iAORG
int 13h
jc Error


call Print_Progress


; Set DATA of INIT1 in App Ram
; ASSUME: DL = 0
mov BX, $pINIT1
mov [BX], DL

mov AH, [$pDriveTable + 3]       ; Get Ram Size in sectors
mov [BX + 1], AH

xor AL, AL
shl AH, 1                        ; Convert Sectors into Bytes
mov [BX + 2], AX                 ; Set Stack
mov SP, AX                       ; Mov AX as app SP


mov AX, [BX + $iAORG]
mov [Jump + 1], AX

; ASSUME: ES = $sINIT1
mov AX, ES
mov SS, AX
mov DS, AX

Jump: jmp $sINIT1:0


Error:
   mov AX, (0Eh << 8) | '*'
   xor BX, BX
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
   and AX, 0xFFF
   xor DH, DH
   mov CL, 36
   div CL
   mov CL, AH
   sub AH, 18
   jb Head0

   mov DH, 1
   mov CL, AH
Head0:
   inc CX
   mov CH, AL
   ret


Print_Progress:
   mov AX, 0xE30
   inc byte [$-2]
   xor BX, BX
   int 10h
   ret

; Bootloader Signature
times 510-($-$$) db 0
dw 0xAA55