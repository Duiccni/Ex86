C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\mybios.asm -o asm\mybios.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\int16h.asm -o asm\int16h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\int13h.asm -o asm\int13h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\int10h.asm -o asm\int10h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\int21h.asm -o asm\int21h.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\floppy.asm -o asm\floppy.bin -f bin
C:\Users\abi37\AppData\Local\bin\NASM\nasm.exe asm\drive_table.asm -o asm\drive_table.bin -f bin

cd C:\"Program Files"\LLVM\bin
.\clang.exe C:\Users\abi37\Documents\Projects\8086_6.1_2D\main.cpp -O -o C:\Users\abi37\Documents\Projects\8086_6.1_2D\test.exe -w -std=c++23 -lUser32 -lGdi32
cd C:\Users\abi37\Documents\Projects\8086_6.1_2D
C:\Users\abi37\Documents\Projects\8086_6.1_2D\test.exe