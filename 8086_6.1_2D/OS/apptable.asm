%define FIRST(ID, INDEX) (ID << 12) | INDEX

; INIT1
dw FIRST(0, 2)          ; Index
db 1                    ; Drive Size
db 1                    ; Ram Size

; FUN2
dw FIRST(1, 3)          ; Index
db 1                    ; Drive Size
db 1                    ; Ram Size

; EXPLORER3
dw FIRST(2, 4)          ; Index
db 1                    ; Drive Size
db 1                    ; Ram Size

; FUN4
dw FIRST(1, 5)          ; Index
db 1                    ; Drive Size
db 1                    ; Ram Size

; FUN5
dw FIRST(1, 6)          ; Index
db 1                    ; Drive Size
db 1                    ; Ram Size

; FUN6
dw FIRST(1, 7)          ; Index
db 1                    ; Drive Size
db 1                    ; Ram Size



times 512-($-$$) db 0