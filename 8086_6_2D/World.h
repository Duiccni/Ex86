#pragma once

#include <Windows.h>

#pragma comment(lib, "Winmm.lib")

#include "Graphics.h"
#include "Colors.h"
#include "Font.h"

#if _INCLUDE_3DH == 1
#include "3Dmath.h"
#endif

constexpr point screen_size = { 720 + 420, 540 };

graphics::surface_ex screen = graphics::surface_ex(screen_size);

struct s_mouse
{
	POINT win_pos;
	point pos;
	point old_pos;
	point delta;
	bool in_screen;
	bool left, middle, right;
	bool t_left, t_middle, t_right;
};

namespace data
{
	constexpr uint target_fps = 10, target_frame_time = 1000 / target_fps, h_target_fps = target_fps >> 1;
	uint tick = 0, delta_time = target_frame_time, performance = 0;
	bool running = true;

	s_mouse mouse = {};
}

constexpr point extra_size = { 16, 39 };

BITMAPINFO bitmap_info;

void update_mouse(HWND hwnd)
{
	GetCursorPos(&data::mouse.win_pos);
	ScreenToClient(hwnd, &data::mouse.win_pos);
	data::mouse.old_pos = data::mouse.pos;
	data::mouse.pos = { data::mouse.win_pos.x, screen.size.y - data::mouse.win_pos.y };
	data::mouse.delta = data::mouse.pos - data::mouse.old_pos;
	data::mouse.in_screen = graphics::is_inside(data::mouse.pos, screen_size);
}

void clear_mouse_tick()
{
	data::mouse.t_left = false;
	data::mouse.t_middle = false;
	data::mouse.t_right = false;
}
