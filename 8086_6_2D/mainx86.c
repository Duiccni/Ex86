#include "basic.c"
#include <string.h>

u8 CIDf[3] = {CFi, IFi, DFi};

inline u16 get_direction(u8 w) {
	return get_sign16(cpu.flags << (15 - DFi)) << w;
}

u8 IO_port = 0;
void (*OUTio[0x100])(u16);
u16 (*INio[0x100])();

void default_out(u16 x) {
	printf("Unk port out: %u\n", IO_port);
	error_no = ERR_IO_DEF;
}

u16 default_in() {
	printf("Unk port in: %u\n", IO_port);
	error_no = ERR_IO_DEF;
	return 0;
}

u8 halt = 0;
u32 tick = 0;

u32 PC_pre1 = 0;
u32 PC_pre2 = 0;

#define SECTOR_SIZE 512
#define SECTORS 18
#define HEADS 2
#define CYLINDERS 80

#define FLOPPY_AMOUNT 1
u8* floppies[FLOPPY_AMOUNT];

u16 get_floppy_sector() {
	u8 sector = cpu.C.l & ~0xC0;
	u16 cylinder = cpu.C.h | ((cpu.C.l & 0xC0) << 8);;
	return (HEADS * cylinder + cpu.D.h) * SECTORS + (sector - 1);
}

u16 read_sectors_in() {
	halt = 1;
	u32 to = cpu.ES + cpu.B.x, from = get_floppy_sector(), size = cpu.A.l * SECTOR_SIZE;
	printf("\nRead from floppy%u:\n From: #%X * 512\n To: %X:%X\n Size: %u * 512\n", cpu.D.l, from, cpu.ES, cpu.B.x, cpu.A.l);
	from *= SECTOR_SIZE;
	if (cpu.D.l >= FLOPPY_AMOUNT || from + size >= 1440 * KB || to >= MB) {
		error_no = ERR_UNK_FLOPPY;
		return 1;
	}
	memcpy(ram + to, floppies[cpu.D.l] + from, size);
	return 0;
}

u8 last_key_pressed = 0;
u16 last_key_press_tick = -1;
u16 get_key_in() {
	printf("\nHalt bcuz of 'in 0' (get the last key input from keyboard).\n");
	halt = 1;
	return last_key_pressed;
}

u16 get_tick_in() {
	return data::tick;
}

void initalize_IO() {
	for (u16 i = 0; i < 0x100; i++) {
		OUTio[i] = default_out;
		INio[i] = default_in;
	}

	INio[0] = get_key_in;
	INio[1] = read_sectors_in;
	INio[2] = get_tick_in;
}

void interrupt(u8 i) {
	LI_add_name('I','N','T');
	hex_to_LI(i, 2);
	last_inst_buf[LI_i++] = ' ';
	hex_to_LI(cpu.A.h, 2);

	set_fbit(IFi, 0);

	get16(stack - 2) = cpu.flags;
	get16(stack - 4) = cpu.CS >> 4;
	cpu.SP -= 6;
	get16(stack) = cpu.IP;

	u16 a = i << 2;
	cpu.IP = get16(a);
	cpu.CS = get16(a + 2) << 4;
}

#define SET_FLAGv(x) (((x) & 0b11010101) | 2)
void tick_cpu() {
	u8 seg_or = DSi, opcode, masked, w;
tick_cpu:
	opcode = fetch8, w = opcode & 1;

	// 0011 1111
	if ((opcode & 0xC0) == 0) {
		u8 mid = opcode >> 3;

		if ((opcode & 0b110) != 0b110) {
			void *to, *from;

			if (opcode & 0b100) {
				to = &cpu.A;
				from = ram + PC;
				cpu.IP += (1 << w);
			} else {
				u8 mod_rm = fetch8;
				to = get_mod_rm(mod_rm, w, seg_or);
				from = get_register(get_modrm_mid(mod_rm), w);
				if (opcode & 2) XCHG(void*, to, from);
			}

			return proc_fn0(w, to, *(u16*)from, mid);
		}

		if (opcode & 0x20) {
			if (w) {
				error_no = ERR_BCD;
				return;
			}

			seg_or = mid;
			goto tick_cpu;
		}

		return push_pop16_seg(mid, w);
	}

	// IO
	if ((opcode & 0b11110100) == 0b11100100) {
		u8 port = (opcode & 0b1000) ? cpu.D.l : fetch8;
		IO_port = port;
		switch (opcode & 0b11) {
		case 0b00: cpu.A.l = INio[port](); return;
		case 0b01: cpu.A.x = INio[port](); return;
		case 0b10:
		case 0b11: OUTio[port](cpu.A.x); return;
		}
	}

	// 0000 0001
	switch (opcode & 0xFE) {
	case 0xE8: {
		LI_add_name('J','M','P','i');
		u16 inc16 = fetch16();
		if (!w) {
			cpu.SP -= 2;
			get16(stack) = cpu.IP;
		}
		cpu.IP += inc16;
		return;
	}
	case 0xF6: {
		static char* F6_names[] = {"TEST", "nul ", "NOT ", "NEG ", "MUL ", "IMUL", "DIV", "IDIV"};
		u8 mod_rm = fetch8, mid = get_modrm_mid(mod_rm);
		void *rm = get_mod_rm(mod_rm, w, seg_or);
		char* name = F6_names[mid];
		LI_add_name(name[0], name[1], name[2], name[3]);
		switch (mid) {
		case 0b000: // TEST
			if (w) set_logical_flags8(*(u8*)rm & fetch8);
			else set_logical_flags16(*(u16*)rm & fetch16());
			return;
		case 0b001: error_no = ERR_UNUSED; return;
		case 0b010: // NOT
			if (w) *(u16*)rm = ~*(u16*)rm;
			else *(u8*)rm = ~*(u8*)rm;
			return;
		case 0b011: // NEG
			if (w) {
				u16 x = -*(u16*)rm;
				*(u16*)rm = x;
				set_fbit(CFi, x != 0);
				set_fbit(OFi, x == 0x8000);
				set_fbit(ZFi, x == 0);
				set_fbit(SFi, x >> 15);
			} else {
				u8 x = -*(u8*)rm;
				*(u8*)rm = x;
				set_fbit(CFi, x != 0);
				set_fbit(OFi, x == 0x80);
				set_fbit(ZFi, x == 0);
				set_fbit(SFi, x >> 7);
			}
			return;
		case 0b100: // MUL
			if (w) {
				u32 result = cpu.A.x * *(u16*)rm;
				cpu.A.x = result;
				cpu.D.x = result >> 16;
				mod_rm = cpu.D.x != 0;
			} else {
				cpu.A.x = cpu.A.l * *(u8*)rm;
				mod_rm = cpu.A.h != 0;
			}
			break;
		case 0b101: // IMUL
			if (w) {
				u32 result = (i16)cpu.A.x * *(i16*)rm;
				cpu.A.x = result;
				cpu.D.x = result >> 16;
				mod_rm = ((((i32)result >> 15) + 1) & -2) == 0;
			} else {
				cpu.A.x = (i8)cpu.A.l * *(i8*)rm;
				mod_rm = ((((i16)cpu.A.x >> 7) + 1) & -2) == 0;
			}
			break;
		case 0b110: // DIV
			if (w) {
				u32 divided = (cpu.D.x << 16) | cpu.A.x;
				u16 divider = *(u16*)rm;
				cpu.A.x = divided / divider;
				cpu.D.x = divided % divider;
			} else {
				u16 divided = cpu.A.x;
				u8 divider = *(u8*)rm;
				cpu.A.l = divided / divider;
				cpu.A.h = divided % divider;
			}
			return;
		case 0b111: // IDIV
			if (w) {
				i32 divided = (cpu.D.x << 16) | cpu.A.x;
				i16 divider = *(u16*)rm;
				cpu.A.x = divided / divider;
				cpu.D.x = divided % divider;
			} else {
				i16 divided = cpu.A.x;
				i8 divider = *(u8*)rm;
				cpu.A.l = divided / divider;
				cpu.A.h = divided % divider;
			}
			return;
		}
		
		set_fbit(CFi, mod_rm);
		set_fbit(OFi, mod_rm);
		return;
	}
	case 0xFE: {
		u8 mod_rm = fetch8, C5 = get_modrm_mid(mod_rm);
		void *rm = get_mod_rm(mod_rm, w, seg_or);
		if (!w) {
			if_error(C5 & 0b110, ERR_UNUSED);
			u8 a = *(u8*)rm, b = get_sign8(C5 << 7);
			u16 c = a + b;
			set_arithmetic_flags8(a, b, c);
			*(u8*)rm = c;
			return;
		}
		if_error(C5 == 0b111, ERR_UNUSED);
		if ((C5 & 0b110) == 0) { // INC/DEC
			if_error((mod_rm & 0xC0) == 0xC0, ERR_MOD11);
			u16 a = *(u16*)rm, b = get_sign16(C5 << 15);
			u32 c = a + b;
			set_arithmetic_flags16(a, b, c);
			*(u16*)rm = c;
			return;
		}
		if (C5 == 0b110) { // PUSH
			if_error((mod_rm & 0xC0) == 0xC0, ERR_MOD11);
			cpu.SP -= 2;
			get16(stack) = *(u16*)rm;
			return;
		}

		LI_add_name('C', 'J', 'f', 'e');

		if (C5 & 1) { // INTER CALL/JMP
			if_error((mod_rm & 0xC0) == 0xC0, ERR_MOD11);

			if (C5 & 2) { // CALL
				get16(stack - 2) = cpu.CS >> 4;
				cpu.SP -= 4;
				get16(stack) = cpu.IP;
			}
	
			// JMP
			cpu.IP = ((u16*)rm)[0];
			cpu.CS = ((u16*)rm)[1] << 4;
			return;
		}

		if (C5 & 2) { // CALL
			cpu.SP -= 2;
			get16(stack) = cpu.IP;
		}

		// JMP
		cpu.IP = *(u16*)rm;
		return;
	}
	case 0x9C: {
		LI_add_name('P','O','P','f');
		if (w) {
			cpu.flags = SET_FLAGv(get16(stack));
			cpu.SP += 2;
		} else {
			cpu.SP -= 2;
			get16(stack) = cpu.flags;
		}
		return;
	}
	case 0xF0:
	case 0xC0:
	case 0xC8: { error_no = ERR_UNUSED; return; }
	case 0xC2: {
		u16 IP = get16(stack);
		cpu.SP += 2;
		if ((opcode & 1) == 0) cpu.SP += fetch16();
		cpu.IP = IP;
		return;
	}
	case 0xCA: {
		u16 IP = get16(stack), CS = get16(stack + 2);
		cpu.SP += 4;
		if ((opcode & 1) == 0) cpu.SP += fetch16();
		cpu.IP = IP;
		cpu.CS = CS << 4;
		return;
	}
	case 0xC4: {
		u8 mod_rm = fetch8;
		if_error((mod_rm & 0xC0) == 0xC0, ERR_LxS_MOD11);
		u16 *mem = (u16*)get_mod_rm(mod_rm, 1, seg_or);
		cpuw[get_modrm_mid(mod_rm)] = mem[0];
		*(w ? &cpu.DS : &cpu.ES) = mem[1] << 4;
		return;
	}
	case 0xC6: {
		u8 mod_rm = fetch8;
		if_error(get_modrm_mid(mod_rm), ERR_UNUSED);
		void *rm = get_mod_rm(mod_rm, w, seg_or);
		if (w) *(u16*)rm = fetch16();
		else *(u8*)rm = fetch8;
		return;
	}
	case 0xD4: { error_no = ERR_BCD; return; }
	}

	// 0000 0011
	switch (opcode & 0xFC) {
	case 0xA0: {
		LI_add_name('M','O','V','a');
		void *to = &cpu.A, *from = cpus[seg_or & 0b11] + fetch16() + ram;
		if (opcode & 2) XCHG(void*, to, from);
		if (w) *(u16*)to = *(u16*)from;
		else *(u8*)to = *(u8*)from;
		return;
	}
	case 0xA4: {
		void *to = cpu.ES + cpu.DI + ram, *from = cpu.DS + cpu.SI + ram;
		switch (opcode & 0b11) {
		case 0b00: *(u8*)to = *(u8*)from; break;
		case 0b01: *(u16*)to = *(u16*)from; break;
		case 0b10: cmp8(*(u8*)to, *(u8*)from); break;
		case 0b11: cmp16(*(u16*)to, *(u16*)from); break;
		}

		u16 v = get_direction(w);
		cpu.SI += v;
		cpu.DI += v;
		return;
	}
	case 0xD0: {
		u8 mod_rm = fetch8;
		void *rm = get_mod_rm(mod_rm, w, seg_or);
		u8 count = (opcode & 2) ? cpu.C.l : 1;
		if (count == 0) return;

		switch (get_modrm_mid(mod_rm)) {
		case 0:
		case 2:
			if (w) {
				u16 b = *(u16*)rm;
				*(u16*)rm = _rotl16(*(u16*)rm, count);
				set_fbit(OFi, (*(u16*)rm ^ b) >> 15);
			} else {
				u8 b = *(u8*)rm;
				*(u8*)rm = _rotl8(*(u8*)rm, count);
				set_fbit(OFi, (*(u8*)rm ^ b) >> 7);
			}
			set_fbit(CFi, *(u8*)rm & 1);
			return;
		case 1:
		case 3:
			if (w) {
				u16 b = *(u16*)rm;
				*(u16*)rm = _rotr16(*(u16*)rm, count);
				set_fbit(OFi, (*(u16*)rm ^ b) >> 15);
				set_fbit(CFi, *(u16*)rm >> 15);
			} else {
				u8 b = *(u8*)rm;
				*(u8*)rm = _rotr8(*(u8*)rm, count);
				set_fbit(OFi, (*(u8*)rm ^ b) >> 7);
				set_fbit(CFi, *(u8*)rm >> 7);
			}
			return;
		case 4:
		case 6:
			if (w) {
				set_fbit(CFi, (*(u16*)rm >> (16 - count)) & 1);
				u16 b = *(u16*)rm;
				*(u16*)rm = *(u16*)rm << count;
				set_fbit(OFi, (*(u16*)rm ^ b) >> 15);
			} else {
				set_fbit(CFi, (*(u8*)rm >> (8 - count)) & 1);
				u8 b = *(u8*)rm;
				*(u8*)rm = *(u8*)rm << count;
				set_fbit(OFi, (*(u8*)rm ^ b) >> 7);
			}
			return;
		case 5:
			if (w) {
				set_fbit(CFi, (*(u16*)rm >> (count - 1)) & 1);
				u16 b = *(u16*)rm;
				*(u16*)rm = *(u16*)rm >> count;
				set_fbit(OFi, (*(u16*)rm ^ b) >> 15);
			} else {
				set_fbit(CFi, (*(u8*)rm >> (count - 1)) & 1);
				u8 b = *(u8*)rm;
				*(u8*)rm = *(u8*)rm >> count;
				set_fbit(OFi, (*(u8*)rm ^ b) >> 7);
			}
			return;
		case 7:
			if (w) {
				set_fbit(CFi, (*(u16*)rm >> (count - 1)) & 1);
				u16 b = *(u16*)rm;
				*(i16*)rm = *(i16*)rm >> count;
				set_fbit(OFi, (*(u16*)rm ^ b) >> 15);
			} else {
				set_fbit(CFi, (*(u8*)rm >> (count - 1)) & 1);
				u8 b = *(u8*)rm;
				*(i8*)rm = *(i8*)rm >> count;
				set_fbit(OFi, (*(u8*)rm ^ b) >> 7);
			}
		return;
		}
	}
	case 0xE0: {
		LI_add_name('L','O','O','P');
		cpu.C.x--;
		cpu.IP++;
		switch (opcode) {
		case 0xE0: if (cpu.C.x && !get_fbit(ZFi)) break; return;
		case 0xE1: if (cpu.C.x && get_fbit(ZFi)) break; return;
		case 0xE2: if (cpu.C.x) break; return;
		case 0xE3: if (!cpu.C.x) break; return;
		}
		cpu.IP += (i8)ram[PC - 1];
		return;
	}
	}
	
	// 0000 0111
	switch (opcode & 0xF8) {
	case 0x90: // XCHG A
		opcode &= 0b111;
		XCHG(u16, cpu.A.x, cpuw[opcode]);
		return;
	case 0xF8: { // CLx, STx
		u8 F = CIDf[(opcode >> 1) & 0b11];
		LI_add_name('F',w | '0','L',to_hex(F));
		set_fbit(F, w);
		return;
	}
	case 0xA8: {
		void *to = cpu.ES + cpu.DI + ram, *from = cpu.DS + cpu.SI + ram;
		u16 v = get_sign16(cpu.flags << (15 - DFi)) << w;

		switch (opcode & 0b111) {
		case 0b000: set_logical_flags8(cpu.A.l & fetch8); return;
		case 0b001: set_logical_flags16(cpu.A.x & fetch16()); return;

		case 0b010: *(u8*)to = cpu.A.l; break;
		case 0b011: *(u16*)to = cpu.A.x; break;
		case 0b110: cmp8(*(u8*)to, cpu.A.l); break;
		case 0b111: cmp16(*(u16*)to, cpu.A.x); break;
		
		case 0b100: cpu.A.l = *(u8*)from; cpu.SI += v; return;
		case 0b101: cpu.A.x = *(u16*)from; cpu.SI += v; return;
		}

		cpu.DI += v;
		return;
	}
	case 0xB0:
		LI_add_name('M','O','V','i');
		LI_add_register(opcode & 0b111);
		opcode &= 0b111;
		cpub[decode_reg8(opcode)] = fetch8;
		return;
	case 0xB8:
		LI_add_name('M','O','V','i');
		LI_add_register(opcode & 0b1111);
		cpuw[opcode & 0b111] = fetch16();
		return;
	case 0xD8: error_no = ERR_ESC; return; // ESC | FPU
	}

	// 0000 1111
	switch (opcode & 0xF0) {
	case 0x40: {
		LI_add_name('I','N','C', to_hex(opcode & 0b1111));
		u8 reg = opcode & 0b111;
		u16 a = cpuw[reg], b = get_sign16(opcode << 12);
		u32 c = a + (i16)b;
		set_arithmetic_flags16(a, b, c);
		cpuw[reg] = c;
		return;
	}
	case 0x50:
		return push_pop16(cpuw + (opcode & 0b111), opcode & 8);
	case 0x60:
		error_no = ERR_UNUSED;
		return;
	case 0x70: // JMP Jcc
		if (conditionCC(opcode & 0xF) & 1) cpu.IP += (i8)ram[PC];
		cpu.IP++;
		return;
	case 0x80: {
		u8 mod_rm = fetch8, mid = get_modrm_mid(mod_rm);
		masked = opcode & 0xC;

		if (masked == 0b1100) {
			if (w) {
				if (opcode & 2) {
					if_error(mid, ERR_UNUSED)
					*(u16*)get_mod_rm(mod_rm, 1, seg_or) = get16(stack);
					cpu.SP += 2;
					return;
				}

				u8 mod = mod_rm >> 6;
				if_error(mod == 0b11, ERR_LEA_MOD11)
				cpuw[mid] = efficient_address(mod, mod_rm & 0b111);
				return;
			}

			if_error(mod_rm & 0x20, ERR_UNUSED)
			u16 *to = (u16*)get_mod_rm(mod_rm, 1, seg_or);
			if (opcode & 2) cpus[mid] = *to << 4;
			else *to = cpus[mid] >> 4;
			return;
		}

		u8 nd = opcode & 2;
		void *to = get_mod_rm(mod_rm, w, seg_or);

		switch (masked) {
		case 0b0000:
			if_error(nd && is_fn0_logical(mid), ERR_SX_LOGIC);
			return proc_fn0(w, to, (nd | (w ^ 1)) ? (i8)fetch8 : fetch16(), mid);
		case 0b0100: {
			void *reg = get_register(mid, w);

			if (nd) {
				if (w) XCHG(u16, *(u16*)to, *(u16*)reg)
				else XCHG(u8, *(u8*)to, *(u8*)reg)
			} else {
				if (w) set_logical_flags16(*(u16*)to & *(u16*)reg);
				else set_logical_flags8(*(u8*)to & *(u8*)reg);
			}

			return;
		}
		case 0b1000: {
			LI_add_name('M','O','V', '0' | nd);
			void *from = get_register(mid, w);
			if (nd) XCHG(void*, to, from);

			if (w) *(u16*)to = *(u16*)from;
			else *(u8*)to = *(u8*)from;

			return;
		}
		}
	}
	}

	switch (opcode) {
	case 0x98: cpu.A.x = (i8)cpu.A.l; return; // CBW
	case 0x99: cpu.D.x = sign_mask16(cpu.A.x); return; // CWD

	case 0xCC: return interrupt(3); // INT
	case 0xCD: return interrupt(fetch8);
	case 0xCE: if (get_fbit(OFi)) interrupt(4); return;
	case 0xCF: // IRET
		cpu.IP = get16(stack);
		cpu.CS = get16(stack + 2) << 4;
		cpu.flags = SET_FLAGv(get16(stack + 4));
		cpu.SP += 6;
		return;

	case 0x9A: // CALL FAR
		get16(stack - 2) = cpu.CS >> 4;
		cpu.SP -= 4;
		get16(stack) = cpu.IP;
	case 0xEA: { // JMP FAR
		LI_add_name('C','J','f');
		u16 IP = fetch16();
		cpu.CS = fetch16() << 4;
		cpu.IP = IP;
		return;
	}

	case 0x9E: *(u8*)&cpu.flags = SET_FLAGv(cpu.A.h); return; // SAHF
	case 0x9F: cpu.A.h = *(u8*)&cpu.flags; return; // LAHF

	case 0xF4: halt = 1; return;
	case 0xF5: xor_bit(cpu.flags, CF); return; // CMC

	case 0x9B: // WAIT
	case 0xD6: error_no = ERR_UNUSED; return;

	case 0xD7: cpu.A.l = ram[cpu.DS + cpu.B.x + cpu.A.l]; return; // XLATB

	case 0xEB: cpu.IP += (i8)fetch8; return;

	case 0xF2: {
	repnz_loop:
		if (cpu.C.x == 0) {
			set_fbit(ZFi, 1);
			cpu.IP++;
			return;
		}
		tick_cpu();
		cpu.C.x--;
		if (get_fbit(ZFi)) return;
		cpu.IP--;
		goto repnz_loop;
	}
	case 0xF3: {
		// cmpsW & scasW
		if ((ram[cpu.CS + cpu.IP] & ~0b1001) == 0xA6) goto repz_loop;
	rep_loop:
		if (cpu.C.x == 0) {
			set_fbit(ZFi, 1);
			cpu.IP++;
			return;
		}
		tick_cpu();
		cpu.C.x--, cpu.IP--;
		goto rep_loop;
	repz_loop:
		if (cpu.C.x == 0) {
			set_fbit(ZFi, 1);
			cpu.IP++;
			return;
		}
		tick_cpu();
		cpu.C.x--;
		if (get_fbit(ZFi) == 0) return;
		cpu.IP--;
		goto repz_loop;
	}
	}
}

FILE* deassemble_file;

void tick_cpu2() {
	if (halt) return;

	LI_i = 0;
	PC_pre2 = PC_pre1;
	PC_pre1 = PC;
	tick++;
	tick_cpu();
	if (error_no) {
		halt = 1;
		printf("\nError: %u\n", error_no);
	}
	last_inst_buf[LI_i] = 0;
	fprintf(deassemble_file, "%s\n", last_inst_buf);
}

#define push(x) ram[pc++] = x;
#define RM(a, b, c) 0b##a##b##c

// cd c:\"Program Files"\LLVM\bin
// .\clang.exe C:\Users\abi37\Documents\Projects\8086_6_2D\mainx86.c -O -o C:\Users\abi37\Documents\Projects\8086_6_2D\test.exe
