C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe bios\mybios.asm -o bin\mybios.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe bios\int16h.asm -o bin\int16h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe bios\int13h.asm -o bin\int13h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe bios\int10h.asm -o bin\int10h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe bios\drive_table.asm -o bin\drive_table.bin -f bin



C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\apptable.asm -o bin\apptable.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\bootloader0.asm -o bin\bootloader0.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\INIT1.asm -o bin\INIT1.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\FUN2.asm -o bin\FUN2.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\EXPLORER3.asm -o bin\EXPLORER3.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\FUN4.asm -o bin\FUN4.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\FUN5.asm -o bin\FUN5.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe OS\FUN6.asm -o bin\FUN6.bin -f bin


:: C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\drive_table.asm -o bin\drive_table.bin -f bin
:: C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\int21h.asm -o asm\int21h.bin -f bin

cd C:\"Program Files"\LLVM\bin
.\clang.exe C:\Users\abi37\Documents\Projects\8086_6.1_2D\main.cpp -O -o C:\Users\abi37\Documents\Projects\8086_6.1_2D\test.exe -w -std=c++23 -lUser32 -lGdi32
cd C:\Users\abi37\Documents\Projects\8086_6.1_2D
C:\Users\abi37\Documents\Projects\8086_6.1_2D\test.exe