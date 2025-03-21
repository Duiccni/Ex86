#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

#define if_error(con, err) if (con) { error_no = err; return; }

#define sign_mask8(x) ((i8)(x) >> 7)
#define sign_mask16(x) ((i16)(x) >> 15)

#define get_sign8(x) (sign_mask8(x) | 1)
#define get_sign16(x) (sign_mask16(x) | 1)

typedef union {
	u16 x;
	struct { u8 l, h; };
} Reg;

struct {
	Reg A, C, D, B;
	u16 SP, BP, SI, DI;
	u32 ES, CS, SS, DS; // shifted by 4
	u16 IP, flags;
} cpu;

void reset_cpu() {
	cpu.IP = cpu.ES = cpu.SS = cpu.DS = 0;
	cpu.flags = 2;
	cpu.CS = 0xFFFF0;
}

#define cpub ((u8*)&cpu)
#define cpuw ((u16*)&cpu)
#define cpus (&cpu.ES)

#define ESi 0
#define CSi 1
#define SSi 2
#define DSi 3

#define AXi 0
#define CXi 1
#define DXi 2
#define BXi 3
#define SPi 4
#define BPi 5
#define SIi 6
#define DIi 7

#define decode_reg8(x) (((x >> 2) | (x << 1)) & 0b111)

#define CFi 0
#define ZFi 6
#define SFi 7
#define TFi 8
#define IFi 9
#define DFi 10
#define OFi 11

#define KB 0x400
#define MB 0x100000

void print_binary(u16 x, u8 c) {
	if (c == 0) return;
loop:
	putchar(((x >> --c) & 1) + '0');
	if (c) goto loop;
}

u8 *ram;

#define get16(addr) *(u16*)((addr) + ram)
#define PC (cpu.CS + cpu.IP)
#define fetch8 ram[cpu.CS + cpu.IP++]

inline u16 fetch16() {
	u16 x = *(u16*)(ram + PC);
	cpu.IP += 2;
	return x;
}

#define get_bit(x, i) (((x) >> (i)) & 1)
#define set_bit(x, i) (x |= (1 << (i)))
#define clear_bit(x, i) (x &= ~(1 << (i)))
#define xor_bit(x, i) (x ^= (1 << (i)))
#define change_bit(x, i, v) (clear_bit(x, i), x |= ((v) << (i)))

#define get_fbit(i) get_bit(cpu.flags, i)
#define set_fbit(i, v) change_bit(cpu.flags, i, v)

inline void set_of8(u8 a, u8 b, u8 c) {
	set_fbit(OFi, (~(a ^ b) & (a ^ c)) >> 7);
}

inline void set_of16(u16 a, u16 b, u16 c) {
	set_fbit(OFi, (~(a ^ b) & (a ^ c)) >> 15);
}

#define putsout(buf) fputs(buf, stdout)

void print_reg(Reg reg) {
	print_binary(reg.h, 8);
	putchar(' ');
	print_binary(reg.l, 8);
	printf(" (%u : %u, %u)\n", reg.h, reg.l, reg.x);
}
void print_regx(Reg reg) {
	print_binary(reg.h, 8);
	putchar(' ');
	print_binary(reg.l, 8);
	printf(" (#%04X)\n", reg.x);
}
void print_word(u16 x) {
	print_binary(x, 16);
	printf(" (#%04X)\n", x);
}
void print_GPs() {
	print_reg(cpu.A);
	print_reg(cpu.C);
	print_reg(cpu.D);
	print_regx(cpu.B);
}
void print_PRs() {
	print_word(cpu.SP);
	print_word(cpu.BP);
	print_word(cpu.SI);
	print_word(cpu.DI);
}
void print_Ss() {
	print_word(cpu.ES >> 4);
	print_word(cpu.CS >> 4);
	print_word(cpu.SS >> 4);
	print_word(cpu.DS >> 4);
}
void print_IP_flags() {
	print_word(cpu.IP);
	print_regx(*(Reg*)&cpu.flags);
}
void print_cpu() {
	print_GPs();
	putchar('\n');
	print_PRs();
	putchar('\n');
	print_Ss();
	putchar('\n');
	print_IP_flags();
}

inline u8 to_hex(u8 x) { return x + (x < 10 ? '0' : 'A' - 10); }
#define print_byte_hex(x) putchar(to_hex(x >> 4)), putchar(to_hex(x & 0b1111))
void f_print_byte_hex(u8 x) {
	if (x == 0) {
		putchar('.');
		putchar(' ');
		return;
	}
	putchar(to_hex(x >> 4));
	putchar(to_hex(x & 0b1111));
}

void print_memory(u8 *mem, u32 size, u8 stack) {
	stack = (1 << stack) - 1;
	for (u32 i = 0; i < size; ++i) {
		if ((i & stack) == 0) printf("\n%04X: ", i);
		f_print_byte_hex(mem[i]);
		putchar(' ');
	}
}

char last_inst_buf[32];
u8 LI_i = 0;

char *regs[] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH", "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};

void LI_add_register(u8 reg) {
	*(u16*)(last_inst_buf + LI_i) = *(u16*)regs[reg];
	last_inst_buf[LI_i + 2] = ' ';
	LI_i += 3;
}

inline void LI_add_name(char a, char b, char c, char d=' ') {
	last_inst_buf[LI_i] = a;
	last_inst_buf[LI_i + 1] = b;
	last_inst_buf[LI_i + 2] = c;
	last_inst_buf[LI_i + 3] = d;
	last_inst_buf[LI_i + 4] = ' ';
	LI_i += 4 + (d != ' ');
}

void hex_to_LI(u32 x, u8 c) {
loop:
	last_inst_buf[LI_i++] = to_hex((x >> (--c << 2)) & 0b1111);
	if (c) goto loop;
}

inline void LI_add_MOD_RM_start(u8 mod, u8 rm, u8 w) {
	last_inst_buf[LI_i++] = '[';
	if (mod == 0b11) return;
	last_inst_buf[LI_i] = (mod >> 1) | '0';
	last_inst_buf[LI_i + 1] = (mod & 1) | '0';
	last_inst_buf[LI_i + 2] = ',';
	last_inst_buf[LI_i + 3] = (rm >> 2) | '0';
	last_inst_buf[LI_i + 4] = ((rm >> 1) & 1) | '0';
	last_inst_buf[LI_i + 5] = (rm & 1) | '0';
	LI_i += 6;
}

inline void LI_add_MOD_RM_end(u32 addr, u8 w) {
	last_inst_buf[LI_i++] = ',';
	hex_to_LI(addr, 6);
	last_inst_buf[LI_i++] = ',';
	hex_to_LI(*(u16*)(ram + addr), (w + 1) << 1);
	last_inst_buf[LI_i] = ']';
	last_inst_buf[LI_i + 1] = ' ';
	LI_i += 2;
}

inline void *get_register(u8 reg, u8 w) {
	LI_add_register(reg | (w << 3));
	return w ? (void*)(cpuw + reg) : (void*)(cpub + decode_reg8(reg));
}

u8 EA_BP = 0;
u16 efficient_address(u8 mod, u8 rm) {
	EA_BP = 0;
	u16 addr = mod == 1 ? (i8)fetch8 : (mod ? fetch16() : 0);

	if (~rm & 0b110) {
		addr += (&cpu.SI)[rm & 1];
		if (rm & 0b100) return addr;
		return addr + (&cpu.B.x)[EA_BP = rm & 2];
	}

	rm &= 1;
	if ((mod | rm) == 0) return fetch16();
	return addr + (&cpu.B.x)[EA_BP = (rm ^ 1) << 1];
}

void *get_mod_rm(u8 b, u8 w, u8 seg) {
	u8 mod = b >> 6, rm = b & 0b111;
	LI_add_MOD_RM_start(mod, rm, w);

	if (mod == 0b11) return get_register(rm, w);

	u32 addr = efficient_address(mod, rm);
	if (((seg & 0b100) | EA_BP) == 0b010) seg = SSi;
	addr += cpus[seg & 0b11];

	LI_add_MOD_RM_end(addr, w);
	return ram + addr;
}

#define XCHG(T, a, b) { T _t = a; a = b; b = _t; }

void set_logical_flags8(u8 c) {
	set_fbit(CFi, 0);
	set_fbit(OFi, 0);
	set_fbit(ZFi, c == 0);
	set_fbit(SFi, c >> 7);
}
void set_logical_flags16(u16 c) {
	set_fbit(CFi, 0);
	set_fbit(OFi, 0);
	set_fbit(ZFi, c == 0);
	set_fbit(SFi, c >> 15);
}

void set_arithmetic_flags8(u8 a, u8 b, u16 c) {
	set_fbit(CFi, (c >> 8) & 1);
	u8 c8 = c;
	set_fbit(ZFi, c8 == 0);
	set_fbit(SFi, c8 >> 7);
	set_of8(a, b, c8);
}
void set_arithmetic_flags16(u16 a, u16 b, u32 c) {
	set_fbit(CFi, (c >> 16) & 1);
	u16 c16 = c;
	set_fbit(ZFi, c16 == 0);
	set_fbit(SFi, c16 >> 15);
	set_of16(a, b, c16);
}

inline void cmp8(u8 a, u8 b) {
	u16 c = a - b;
	set_arithmetic_flags8(a, b, c);
}
inline void cmp16(u16 a, u16 b) {
	u32 c = a - b;
	set_arithmetic_flags16(a, b, c);
}

u8 is_fn0_logical(u8 fn0) {
	return (fn0 & 0b101) == 0b100 || fn0 == 0b001;
}

typedef u16 fn0_8(u8, u8);
typedef u32 fn0_16(u16, u16);

#define CF (cpu.flags & 1)

u16 _0_ADD8(u8 x, u8 y) { return x + y; }
u16 _1_OR8 (u8 x, u8 y) { return x | y; }
u16 _2_ADC8(u8 x, u8 y) { return x + y + CF; }
u16 _3_SBB8(u8 x, u8 y) { return x - y - CF; }
u16 _4_AND8(u8 x, u8 y) { return x & y; }
u16 _5_SUB8(u8 x, u8 y) { return x - y; }
u16 _6_XOR8(u8 x, u8 y) { return x ^ y; }

u32 _0_ADD16(u16 x, u16 y) { return x + y; }
u32 _1_OR16 (u16 x, u16 y) { return x | y; }
u32 _2_ADC16(u16 x, u16 y) { return x + y + CF; }
u32 _3_SBB16(u16 x, u16 y) { return x - y - CF; }
u32 _4_AND16(u16 x, u16 y) { return x & y; }
u32 _5_SUB16(u16 x, u16 y) { return x - y; }
u32 _6_XOR16(u16 x, u16 y) { return x ^ y; }

fn0_8 *fn0s8[8] = {
	_0_ADD8,
	_1_OR8,
	_2_ADC8,
	_3_SBB8,
	_4_AND8,
	_5_SUB8,
	_6_XOR8,
	_5_SUB8 // CMP
};

fn0_16 *fn0s16[8] = {
	_0_ADD16,
	_1_OR16,
	_2_ADC16,
	_3_SBB16,
	_4_AND16,
	_5_SUB16,
	_6_XOR16,
	_5_SUB16 // CMP
};

char *fn0str[8] = { "ADD", "OR ", "ADC", "SBB", "AND", "SUB", "XOR", "CMP" };

#define get_modrm_mid(x) ((x >> 3) & 0b111)

#define stack (cpu.SS + cpu.SP)

inline void push_pop16(u16 *x, u8 b) {
	LI_add_name('P','O','P','0' | b);
	if (b) {
		cpu.SP += 2;
		*x = get16(stack - 2);
	} else {
		get16(stack - 2) = *x;
		cpu.SP -= 2;
	}
}

#define ERR_BCD 1
#define ERR_UNUSED 2
#define ERR_LEA_MOD11 3
#define ERR_SX_LOGIC 4
#define ERR_LxS_MOD11 5
#define ERR_ESC 6
#define ERR_MOD11 7
#define ERR_IO_DEF 8
#define ERR_POP_CS 9
#define ERR_UNK_FLOPPY 10

u8 error_no = 0;

inline void push_pop16_seg(u8 seg, u8 b) {
	LI_add_name('P','O','P','S');
	if (b) {
		if_error(seg == 1, ERR_POP_CS);
		cpus[seg] = get16(stack) << 4;
		cpu.SP += 2;
	} else {
		cpu.SP -= 2;
		get16(stack) = cpus[seg] >> 4;
	}
}

// use only the first bit
inline u8 _conditionCC(u8 cc) {
	switch (cc) {
	case 0b000: return cpu.flags >> OFi;
	case 0b001: return cpu.flags;
	case 0b010: return cpu.flags >> ZFi;
	case 0b011: return (cpu.flags >> ZFi) | cpu.flags;
	case 0b100: return cpu.flags >> SFi;
	default:
		error_no = ERR_BCD;
		return 0;
	case 0b111: cc = cpu.flags >> ZFi;
	case 0b110: return ((cpu.flags >> SFi) ^ (cpu.flags >> OFi)) | cc;
	}
}

// use only the first bit
inline u8 conditionCC(u8 cc) {
	cc ^= _conditionCC(cc >> 1);
	last_inst_buf[LI_i++] = 'J';
	last_inst_buf[LI_i++] = (cc & 1) | '0';
	return error_no ? 0 : cc;
}

void proc_fn0(u8 w, void *to, u16 b, u8 fn0) {
	LI_add_name(fn0str[fn0][0], fn0str[fn0][1], fn0str[fn0][2]);
	if (w) {
		u16 a = *(u16*)to;
		u32 c = fn0s16[fn0](a, b);

		if (fn0 != 7) *(u16*)to = c;

		if (is_fn0_logical(fn0)) set_logical_flags16(c);
		else set_arithmetic_flags16(a, b, c);
	} else {
		u8 a = *(u8*)to;
		u16 c = fn0s8[fn0](a, b);

		if (fn0 != 7) *(u8*)to = c;

		if (is_fn0_logical(fn0)) set_logical_flags8(c);
		else set_arithmetic_flags8(a, b, c);
	}
}
