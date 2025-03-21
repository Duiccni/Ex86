# Ex86
Mine x86 emulator written in C combined with my old C++ Graphic library

- F1: Tick (stop on mark)
- F2: Mark CS:IP
- F3: Tick
- F4: clear halt
- F5: dump ram
- F6: tick 0x10 times (outside bios part (0xF000:0000-0xFFFF:0000))
- F7: tick until gets out of bios part
- F8: tick 0x100 times and ignore halt instruction
- F9: Tick if 0x7C00 + 512 > CS:IP > 0x7C00
  
Files:
- Main File: main.cpp
- Main x86 emulator File: mainx86.c

#### EXPLORER3.APP
![image](https://github.com/user-attachments/assets/820904f6-0b6a-4aba-9ba9-4f18c83ec8c8)
![image](https://github.com/user-attachments/assets/54fc064a-51cb-4564-815f-e1d1621f340a)

![image](https://github.com/user-attachments/assets/3abd16f2-2c40-4a64-87ce-58c7205a5626)

![image](https://github.com/user-attachments/assets/ef5e6945-0ff3-4bcd-a9e5-a12e50828342)

![image](https://github.com/user-attachments/assets/1a15dae0-81b0-4e47-a515-3e7756454b3b)
