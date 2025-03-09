#define _INCLUDE_3DH 0

#include "World.h"

#include "mainx86.c"

LRESULT CALLBACK window_proc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	switch (u_msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		data::running = false;
		return 0;
	case WM_LBUTTONDOWN:
		data::mouse.t_left = true;
		data::mouse.left = true;
		return 0;
	case WM_LBUTTONUP:
		data::mouse.left = false;
		return 0;
	case WM_MBUTTONDOWN:
		data::mouse.t_middle = true;
		data::mouse.middle = true;
		return 0;
	case WM_MBUTTONUP:
		data::mouse.middle = false;
		return 0;
	case WM_RBUTTONDOWN:
		data::mouse.t_right = true;
		data::mouse.right = true;
		return 0;
	case WM_RBUTTONUP:
		data::mouse.right = false;
		return 0;
	case WM_KEYDOWN:
		if (w_param == VK_SPACE) {
			tick_cpu2();
			return 0;
		}
		if (w_param == VK_F1) {
			for (int i = 0; i < 1000; i++)
				tick_cpu2();
			return 0;
		}
	default:
		return DefWindowProcW(hwnd, u_msg, w_param, l_param);
	}
}

char itb_buffer[17];

char* int_to_binary_buf(u16 x, u8 c) {
	u8 i = 0;
loop:
	itb_buffer[i++] = ((x >> --c) & 1) + '0';
	if (c) goto loop;
	itb_buffer[i] = '\0';
	return itb_buffer;
}

char* int_to_hex_buf(u16 x, u8 c) {
	u8 i = 0;
loop:
	itb_buffer[i++] = to_hex((x >> (--c << 2)) & 0b1111);
	if (c) goto loop;
	itb_buffer[i] = '\0';
	return itb_buffer;
}

char* register_names[14] = {
	"AX", "CX", "DX", "BX",
	"SP", "BP", "SI", "DI",
	"ES", "CS", "SS", "DS",
	"IP", "Flags"
};

void print_register(char* name, u16 x, point pos) {
	pos = font::draw_string(pos, name, C_white, screen);
	pos = font::draw_string(pos, ": ", C_white, screen);
	pos = font::draw_string(pos, int_to_binary_buf(x >> 8, 8), 0xFF4080, screen);
	pos = font::draw_string(pos, " ", 0xFF4080, screen);
	pos = font::draw_string(pos, int_to_binary_buf(x, 8), 0xFF4080, screen);
	pos = font::draw_string(pos, " (#", C_white, screen);
	pos = font::draw_string(pos, int_to_hex_buf(x, 4), 0xFF4080, screen);
	font::draw_string(pos, ")", C_white, screen);
}

u8 text_or_graphic_mode = 1;
void OUT_graphic_module(u16 al) {
	text_or_graphic_mode = al & 1;
}

#define VGA_X 320
#define VGA_Y 200

#define VGA_Xs (VGA_X * 2)
#define VGA_Ys (VGA_Y * 2)
#define VGA_BL_X 480
#define VGA_BR_X VGA_BL_X + VGA_Xs

void VGA() {
	graphics::draw::_straight_line(VGA_BL_X - 1, VGA_BR_X + 1, 29, false, C_white, screen);
	graphics::draw::_straight_line(VGA_BL_X - 1, VGA_BR_X + 1, 30 + VGA_Ys, false, C_white, screen);

	graphics::draw::_straight_line(30, 30 + VGA_Ys, VGA_BL_X - 1, true, C_white, screen);
	graphics::draw::_straight_line(30, 30 + VGA_Ys, VGA_BR_X, true, C_white, screen);

	u8 *px = ram + 0xA0000;
	for (int y = VGA_Ys; y;) {
		y -= 2;
		for (int x = 0; x < VGA_Xs; x += 2, ++px) {
			point p = {x + VGA_BL_X, y + 30};
			u8 s = *px;
			color_t c = s | ((u8)(~s) << 8);
			graphics::set_sure_pixel(p, c, screen);
			graphics::set_sure_pixel(p + P(1, 0), c, screen);
			graphics::set_sure_pixel(p + P(0, 1), c, screen);
			graphics::set_sure_pixel(p + P(1, 1), c, screen);
		}
	}
}

#define TB_L_X VGA_BR_X + 30
#define TB_T_Y (screen_size.y - 30)
void text_box() {
	graphics::draw::fill_rect({TB_L_X, TB_T_Y + 20}, {TB_L_X + 1040, TB_T_Y - 500 + 20}, 0x30, screen);
	u8 *ch = ram + 0xB8000;
	for (u8 y = 0; y < 25; y++)
		for (u8 x = 0; x < 80; x++, ch += 2)
			font::unsafe_draw_char(ch[0], {TB_L_X + x * font::mfdim_xinc, TB_T_Y - y * 20}, (ch[1] << 4) | 0xFF0000, screen);
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd
)
{
#pragma region
	graphics::init();
	if (font::init())
	{
		MessageBoxW(nullptr, L"'font.bin' file cant found!", L"Error", MB_OK);
		return 0;
	}

	WNDCLASSW wc = {};
	wc.lpfnWndProc = window_proc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"6.1";

	if (!RegisterClassW(&wc))
	{
		MessageBoxW(nullptr, L"Failed to register Window Class!", L"Error", MB_OK);
		return 0;
	}

	HWND window = CreateWindowExW(
		0,
		wc.lpszClassName,
		wc.lpszClassName,
		WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		30, 30,
		// CW_USEDEFAULT, CW_USEDEFAULT,
		screen_size.x + extra_size.x, screen_size.y + extra_size.y,
		nullptr, nullptr, hInstance, nullptr
	);

	if (window == nullptr)
	{
		UnregisterClassW(wc.lpszClassName, hInstance);

		MessageBoxW(nullptr, L"Failed to create Window!", L"Error", MB_OK);
		return 0;
	}

	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = screen_size.x;
	bitmap_info.bmiHeader.biHeight = screen_size.y;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	HDC hdc = GetDC(window);

	ShowWindow(window, nShowCmd);
	UpdateWindow(window);

	FILE* fp = nullptr;
	if constexpr (CONSOLE)
	{
		AllocConsole();
		freopen_s(&fp, "CONOUT$", "w", stdout);

		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		SMALL_RECT windowSize = { 0, 0, 108, 38 };
		SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
	}

	MSG msg = {};
	DWORD start_time;
#pragma endregion

// cd c:\"Program Files"\LLVM\bin
// .\clang.exe C:\Users\abi37\Documents\Projects\8086_6_2D\main.cpp -O -o C:\Users\abi37\Documents\Projects\8086_6_2D\test.exe

	ram = (u8*)malloc(MB);

	reset_cpu();
	initalize_IO();
	OUTio[0xD8] = &OUT_graphic_module; // 3D8h (mod 0x100)
	last_inst_buf[0] = 0;

	/*
	u32 pc = 0xFFFF0;
#define push(x) ram[pc++] = x;

	push(0xB8); // mov ax, 0xA000
	push(0x00);
	push(0xA0);

	push(0x8E); // mov ds, ax
	push(0xD8);

	push(0xB1); // mov cl, 7
	push(0x07);

	push(0x89); // mov ax, bx
	push(0xD8);

	push(0xD3); // shr ax, cl
	push(0xE8);

	push(0x3E); // mov BYTE ds:[bx], al
	push(0x88);
	push(0x07);

	push(0x43); // inc bx

	push(0xE9); // jmp loop
	push(0xF5);
	push(0xFF);
	*/

{
	FILE *IBM_5160_bios = fopen("BIOS_5160_09MAY86_U19_62X0819_68X4370_27256_F000.BIN","rb");
	fread(ram + 0xF0000, 1, KB * 32, IBM_5160_bios);
	fclose(IBM_5160_bios);

	IBM_5160_bios = fopen("BIOS_5160_09MAY86_U18_59X7268_62X0890_27256_F800.BIN","rb");
	fread(ram + 0xF8000, 1, KB * 32, IBM_5160_bios);
	fclose(IBM_5160_bios);
	
	system("pause");
}

/*
{
	FILE *test = fopen("test.bin","rb");
	fread(ram + 0xFFFF0, 1, 100, test);
	fclose(test);
}
*/

#pragma region
	while (data::running)
	{
		start_time = timeGetTime();
		while (PeekMessageW(&msg, window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		update_mouse(window);

		C_black >> screen;

		graphics::draw::fill_rect({30, 30}, {53 + font::max_font_dim.x * 4, 50 + font::max_font_dim.y}, halt ? C_green : C_red, screen);
		font::draw_string({40, 40}, "HALT", C_white, screen);

		font::draw_string(font::draw_string({30, 80}, "Graphic mode: ", C_white, screen),
			text_or_graphic_mode ? "Graphic" : "Text", C_lime, screen);
		
		font::draw_string(font::draw_string({120, 40}, "Cpu Tick: ", C_white, screen), int_to_hex_buf(tick, 6), C_lime, screen);

		font::draw_string({30, 80}, last_inst_buf, C_green, screen);

	{
		for (u8 i = 0; i < 8; i++)
			print_register(register_names[i], cpuw[i],
				{ 30, (screen_size.y - 40) - (font::max_font_dim.y + 3) * (i + (i >> 2)) });

		for (u8 i = 0; i < 4; i++)
			print_register(register_names[i + 8], cpus[i] >> 4,
				{ 30, (screen_size.y - 40 - (font::max_font_dim.y + 3) * 10) - (font::max_font_dim.y + 3) * i });

		for (u8 i = 0; i < 2; i++)
			print_register(register_names[i + 12], (&cpu.IP)[i],
			{ 30, (screen_size.y - 40 - (font::max_font_dim.y + 3) * 15) - (font::max_font_dim.y + 3) * i });
	}
		
		font::draw_string({ 30 + font::mfdim_xinc * 11, screen_size.y - 40 - (font::max_font_dim.y + 3) * 17 },
			"ODIT SZ     C", C_cyan, screen);

		VGA();
		text_box();

		if ((data::tick & 0b1111) == 0 && halt == 0) {
			system("cls");

			u32 pc = PC, addr = (pc & ~0b111111) - 0x20;
			printf("Tick: %u\n\n0x%X:", data::tick, addr);
			for (int i = 0; i < 0x100; ++i, ++addr) {
				if ((i & 0b1111) == 0) printf("\n%04X: ", i);

				if (addr == pc) putsout("\033[41m");
				else if (addr == PC_pre1) putsout("\033[42m");
				else if (addr == PC_pre2) putsout("\033[43m");

				f_print_byte_hex(ram[addr]);
				putsout(" \033[0m");
			}

			putsout("\n\n0x00000:");
			print_memory(ram, 0x200, 5);
		}

		StretchDIBits(
			hdc,
			0, 0, screen_size.x, screen_size.y,
			0, 0, screen_size.x, screen_size.y,
			screen.buffer, &bitmap_info,
			DIB_RGB_COLORS, SRCCOPY
		);

		clear_mouse_tick();

		data::performance = timeGetTime() - start_time;
		if (data::performance < data::target_frame_time)
		{
			Sleep(data::target_frame_time - data::performance);
			data::delta_time = data::target_frame_time;
		}
		else data::delta_time = data::performance;
		++data::tick;
	}

	ReleaseDC(window, hdc);
	DestroyWindow(window);
	UnregisterClassW(wc.lpszClassName, hInstance);

	if constexpr (CONSOLE)
	{
		if (fp) fclose(fp);
		FreeConsole();
	}

	free(ram);

	free(screen.buffer);
	screen.buffer = nullptr;
	graphics::clean_up();
	font::clean_up();

	return 0;
#pragma endregion
}