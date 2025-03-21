cpu 8086

org 4

%define $sProcTable 0x40
%define $pProcTable ($sProcTable << 4)
%define $sDriveTable 0x60
%define $pDriveTable ($sDriveTable << 4)
%define $pGS 0xA00

%define $pBL_COMMON 0x7C7B
%define $iBL_COMMON 0x22

%define $iAORG 4

%define Error FileEnd
%define End (FileEnd + 7)
%define ToCHS (FileEnd + 10)

IP:   dw FirstEntry
Type: db 1

db "INIT1  ."

FirstEntry:

xor BX, BX
mov DS, BX
mov [BX + 2], ES
mov word [BX], SysInt  ; Set Int Table

cld
mov SI, $pBL_COMMON
mov DI, FileEnd
mov CX, $iBL_COMMON >> 1
rep movsw

mov CS:[Type], BL    ; Set As Passive

xor AX, AX
jmp FS_L0
L0:
   inc AX
   test AH, AH
   jnz L0_End  ; Loaded all
FS_L0:
   xor BX, BX
   mov ES, BX

   mov BX, AX
   shl BX, 1
   shl BX, 1

   mov DX, ES:[$pDriveTable + BX]
   test DX, DX
   jz L0_End   ; Zero so end

   and DH, 0xF0
   jz L0    ; Its itself

   push AX
   push DX
   
   mov DX, CS
   mov DS, DX

   mov BL, 3
   mov AH, AL
   int 0

   pop DX
   cmp DH, 0x20
   jne CNT
   mov CS:[NextApp], AH
CNT:
   pop AX
   jmp L0

L0_End:

push CS
pop DS

mov word [IP], Entry

Entry:

; ASSUME cld
xor AH, AH
mov AL, [NextApp]
shl AX, 1
mov SI, AX

mov AX, $sProcTable
mov DS, AX
xor DI, DI

FindNextApp:
   lodsw
   and SI, 0x1FE
   test AX, AX
   jz FindNextApp       ; Find Proc
   mov ES, AX           ; Get Proc segment
   mov CL, ES:[DI + 6]  ; Get Type
   test CL, 1
   jz FindNextApp       ; Check if its Active (First bit of type)

push CS
pop DS

; Set back to [NextApp]
mov CX, SI
shr CX, 1   ; CH = 0
mov [NextApp], CL

; Run Next App
; ASSUME: DI = 0, DS = CS
jmp LocalRunES

NextApp: db 1

IntTable:                        ; BL
   db Return - ReturnNCIP        ; 0
   db Exit - ReturnNCIP          ; 1
   db Alloc - ReturnNCIP         ; 2
   db Load - ReturnNCIP          ; 3
   db Run - ReturnNCIP           ; 4
   db ReturnNCIP - ReturnNCIP    ; 5

SysInt:  ; BL
   mov SI, SP     ; Store SP
   xor DI, DI     ; Zero DI for future use
   mov DH, [DI]   ; Get Caller's PID

   mov SS, DI     ; Set Global Stack
   mov SP, $pGS

   xor BH, BH
   mov BL, CS:[IntTable + BX]
   mov CS:[JmpInc], BL

   db 0xEB
JmpInc: db 0

ReturnNCIP:
   mov BL, CS:[DI]
   jmp LocalRunBL_SP
Return:
   ; KNOW: DI = 0, BH = 0
   mov AH, CS:[DI]
Run:           ; IN: AH = PID
   mov BL, AH

   mov AX, [SI]
   mov [DI + 4], AX
LocalRunBL_SP:
   add SI, 6
   mov [DI + 2], SI

LocalRunBL:    ; IN: BL = PID
   ; KNOW: SS = 0, DI = 0
   ; ASSUME: BH = 0
   ; ASSUME: (pS:[$iType] & 1) = 1
   shl BX, 1
   mov ES, SS:[$pProcTable + BX]

LocalRunES:
   push CS
   pop DS

   mov AX, ES:[$iAORG]
   mov [Jump + 1], AX   ; GET IP

   mov AX, ES
   mov [Jump + 3], AX   ; GET CS
   mov SS, AX
   mov DS, AX
   mov SP, [DI + 2]

Jump: jmp 0:0

Exit:
   ; KNOW: BH = 0, SS = 0, DI = 0
   mov BL, DH
   shl BX, 1
   mov SS:[$pProcTable + BX], DI

   xor BH, BH
   mov BL, CS:[DI]
   jmp LocalRunBL


Alloc:
   push ES

   call LocalAlloc

   pop SS
   mov SP, SI
   iret


; IN:
;  AH: App index
; OUT:
;  AH: PID
;  ES: Loaded Segment
Load:
   mov BL, AH
   shl BX, 1
   shl BX, 1

   mov DH, SS:[$pDriveTable + BX + 3]

   push DS
   push BX
   call LocalAlloc
   pop BX
   push AX
   push DS

   mov AX, $sDriveTable
   mov DS, AX
   mov AX, [BX]

   call ToCHS
   mov AL, [BX + 2]
   
   pop ES
   xor DL, DL
   mov AH, 2
   mov BX, $iAORG
   int 13h
   jc Error

   pop AX
   pop SS
   mov SP, SI
   iret


; IN:
;  DH: Size (in sectors)
; OUT:
;  AH: PID
;  DS: Allocated Segment
;  DX: Size (in segments)
align 2
NextSector: dw 0xC0
LocalAlloc:
   cld
   xor AX, AX
   mov ES, AX
   mov DI, $pProcTable
   mov CX, 0x100
   repnz scasw

   mov AX, ES:[DI - 2]
   test AX, AX
   jnz Error

   mov DS, CS:[NextSector]
   mov ES:[DI - 2], DS

   mov AH, CL
   not AH

   xor BX, BX
   mov DL, AH
   mov [BX], DX
   mov [BX + 6], BL

   xor DL, DL
   shl DH, 1
   mov [BX + 2], DX

   mov CL, 4
   shr DX, CL
   add CS:[NextSector], DX

   ret


FileEnd:

times 0x200-($-$$) db 0