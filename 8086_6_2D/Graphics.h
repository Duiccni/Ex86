#pragma once

#include <numeric>

#include "Definations.h"

namespace graphics
{
	inline color_t rgb_color(uchar r, uchar g, uchar b)
	{
		static color_t color = C_alpha;
		static const auto color_ptr = reinterpret_cast<uchar* const>(&color);
		color_ptr[0] = b;
		color_ptr[1] = g;
		color_ptr[2] = r;
		return color;
	}

	inline color_t hsv_to_rgb(int h, uchar s, uchar v)
	{
		h = MODULO(h, 360);

		const uchar c = v * s / UINT8_MAX, x = c - c * fabsf(fmodf(static_cast<float>(h) * FLOAT1DIV60, 2.0f) - 1.0f);

		static color_t color = C_alpha;
		static const auto color_ptr = reinterpret_cast<uchar* const>(&color);
		color_ptr[0] = v - c;
		color_ptr[1] = color_ptr[0];
		color_ptr[2] = color_ptr[0];

		const uchar i = (h / 60 + 1) % 6;

		color_ptr[2 - (i >> 1)] += c;
		color_ptr[i % 3] += x;

		return color;
	}

	struct surface
	{
		color_t* buffer, * end;
		point size;
		size_t buffer_size;

		surface() : buffer{ nullptr }, end{ nullptr }, size{ 0, 0 }, buffer_size{ 0 } { return; }

		surface(point size, bool create_buffer);
		~surface();
	};

	struct surface_ex : surface
	{
		point half_size;
		int smaller_dim, smaller_dim_h;

		surface_ex(point size, bool create_buffer);
	};

	inline surface_ex::surface_ex(point size, bool create_buffer = true)
	{
		this->size = size;
		half_size = size >> 1;
		smaller_dim = min(size.x, size.y);
		smaller_dim_h = smaller_dim >> 1;

		buffer_size = SIZE_MUL(size.x, size.y);
		if (create_buffer)
		{
			buffer = TYPE_MALLOC(color_t, buffer_size);
			end = buffer + buffer_size;
			return;
		}
		buffer = nullptr;
		end = nullptr;
	}

	inline surface::surface(point size, bool create_buffer = true) : size{ size }
	{
		buffer_size = SIZE_MUL(size.x, size.y);
		if (create_buffer)
		{
			buffer = TYPE_MALLOC(color_t, buffer_size);
			end = buffer + buffer_size;
			return;
		}
		buffer = nullptr;
		end = nullptr;
	}

	inline surface::~surface()
	{
		free(buffer);
		buffer = nullptr;
	}

	using csr = const surface&;
	using csrEx = const surface_ex&;

	inline void copy(csr src, csr dest) { memcpy(dest.buffer, src.buffer, src.buffer_size << 2); }
	inline void operator >>(csr src, csr dest) { memcpy(dest.buffer, src.buffer, src.buffer_size << 2); }
	inline void clear(csr dest) { memset(dest.buffer, 0, dest.buffer_size << 2); }

	inline void fill(color_t color, csr dest)
	{
		if (color == C_black)
		{
			clear(dest);
			return;
		}
		for (color_t* px = dest.buffer; px < dest.end; ++px)
			*px = color;
	}

	inline void operator >>(color_t color, csr dest) { fill(color, dest); }

	inline void clamp_to_surface(point& p, csr surf)
	{
		p.x = std::clamp(p.x, 0, surf.size.x);
		p.y = std::clamp(p.y, 0, surf.size.y);
	}

	constexpr size_t CACHE_BUFFER_SIZE = 1920 * 1080;
	static auto cache_buffer = surface();
	static uchar* cache_buffer_uchar;

	void init()
	{
		cache_buffer.buffer = TYPE_MALLOC(color_t, CACHE_BUFFER_SIZE);
		cache_buffer_uchar = reinterpret_cast<uchar*>(cache_buffer.buffer);
	}

	void clean_up()
	{
		free(cache_buffer.buffer);
		cache_buffer.buffer = nullptr;
		cache_buffer_uchar = nullptr;
	}

	constexpr option_t
		RBIS_AUTO = 0,
		RBIS_NORMAL = 3,
		RBIS_ALPHA = 4,
		RBIS_24B = RBIS_NORMAL,
		RBIS_32B = RBIS_ALPHA;

	surface* read_binary_into_surface(const char* file_path, option_t alpha = RBIS_AUTO)
	{
		static const uchar* src;
		static FILE* file;
		static point dim;

		if (fopen_s(&file, file_path, "rb") != SUCCESS)
			return nullptr;

		fread(&dim.x, 4, 2, file);

		if (SIZE_MUL(dim.x, dim.y) > CACHE_BUFFER_SIZE)
		{
			fclose(file);
			return nullptr;
		}

		if (alpha == RBIS_AUTO)
		{
			const char* str_end = file_path;
			while (*str_end != 0) ++str_end;
			alpha = (str_end[-1] == '2' && str_end[-2] == '3') ? RBIS_32B : RBIS_24B;
		}

		const auto dest = new surface(dim);

		for (auto px = dest->buffer; px < dest->end;)
		{
			fread(cache_buffer_uchar, alpha, dim.x, file);
			src = cache_buffer_uchar;
			for (const color_t* x_end = px + dim.x; px < x_end; ++px, src += alpha)
				*px = *reinterpret_cast<const uint*>(src);
		}

		fclose(file);
		return dest;
	}

	void reverse_colors(csr surf)
	{
		for (auto px = surf.buffer; px < surf.end; ++px)
			*px = ~*px;
	}

	void black_and_white(csr surf)
	{
		for (auto px = surf.buffer; px < surf.end; ++px)
		{
			const auto shade = UCHAR_PTR(px);
			*px = ((shade[0] + shade[1] + shade[2]) > 0x17E) ? C_white : C_black;
		}
	}

	void gray_scale(csr surf)
	{
		for (auto px = UCHAR_PTR(surf.buffer), end = UCHAR_PTR(surf.end); px < end; px += 4)
		{
			const uchar color = (px[0] + px[1] + px[2]) / 3;
			px[0] = color;
			px[1] = color;
			px[2] = color;
		}
	}

	void checkers(csr surf, int size, color_t color = C_black)
	{
		static point err;
		color_t start_color = color;
		err.y = 1;
		for (auto px = surf.buffer; px < surf.end; ++err.y)
		{
			color = start_color;
			err.x = 1;
			for (auto x_end = px + surf.size.x; px < x_end; ++px, ++err.x)
			{
				*px = color;
				if (err.x == size)
				{
					color = ~color;
					err.x = 0;
				}
			}
			if (err.y == size)
			{
				start_color = ~start_color;
				err.y = 0;
			}
		}
	}

	bool blur(csr surf)
	{
		if (surf.buffer_size > CACHE_BUFFER_SIZE) return true;
		cache_buffer.end = cache_buffer.buffer + surf.buffer_size;

		surf >> cache_buffer;

		const int y_step = surf.size.x << 2, ny_step = -y_step,
			y_step1n = y_step - 4, y_step2n = y_step - 8,
			y_step1o = y_step | 1, y_step2o = y_step | 2,
			ny_step1o = ny_step | 1, ny_step2o = ny_step | 2;

		// Bottom-Left
		auto dp = UCHAR_PTR(surf.buffer);
		dp[0] = (dp[0] + dp[4] + dp[y_step]) / 3;
		dp[1] = (dp[1] + dp[5] + dp[y_step1o]) / 3;
		dp[2] = (dp[2] + dp[6] + dp[y_step2o]) / 3;

		// Bottom-Right
		dp += y_step1n;
		dp[0] = (dp[0] + dp[-4] + dp[y_step]) / 3;
		dp[1] = (dp[1] + dp[-3] + dp[y_step1o]) / 3;
		dp[2] = (dp[2] + dp[-2] + dp[y_step2o]) / 3;

		// Top-Right
		dp = UCHAR_PTR(surf.end - 1);
		dp[0] = (dp[0] + dp[-4] + dp[ny_step]) / 3;
		dp[1] = (dp[1] + dp[-3] + dp[ny_step1o]) / 3;
		dp[2] = (dp[2] + dp[-2] + dp[ny_step2o]) / 3;

		// Top-Left
		dp -= y_step1n;
		dp[0] = (dp[0] + dp[4] + dp[ny_step]) / 3;
		dp[1] = (dp[1] + dp[5] + dp[ny_step1o]) / 3;
		dp[2] = (dp[2] + dp[6] + dp[ny_step2o]) / 3;

		// Bottom & Top Middle
		for (auto dpb = UCHAR_PTR(surf.buffer + 1), dpt = UCHAR_PTR(surf.end) - y_step1n, end = dpb + y_step2n,
			spb = UCHAR_PTR(cache_buffer.buffer + 1), spt = UCHAR_PTR(cache_buffer.end) - y_step1n;
			dpb < end; dpb += 4, dpt += 4, spb += 4, spt += 4)
		{
			dpb[0] = (spb[0] + spb[4] + spb[-4] + spb[y_step]) >> 2;
			dpb[1] = (spb[1] + spb[5] + spb[-3] + spb[y_step1o]) >> 2;
			dpb[2] = (spb[2] + spb[6] + spb[-2] + spb[y_step2o]) >> 2;

			dpt[0] = (spt[0] + spt[4] + spt[-4] + spt[ny_step]) >> 2;
			dpt[1] = (spt[1] + spt[5] + spt[-3] + spt[ny_step1o]) >> 2;
			dpt[2] = (spt[2] + spt[6] + spt[-2] + spt[ny_step2o]) >> 2;
		}

		// Left & Right Middle
		for (auto dpb = UCHAR_PTR(surf.buffer + surf.size.x), dpt = dpb + y_step1n, end = UCHAR_PTR(surf.end - surf.size.x),
			spb = UCHAR_PTR(cache_buffer.buffer + surf.size.x), spt = spb + y_step1n;
			dpb < end; dpb += y_step, dpt += y_step, spb += y_step, spt += y_step)
		{
			dpb[0] = (spb[0] + spb[4] + spb[y_step] + spb[ny_step]) >> 2;
			dpb[1] = (spb[1] + spb[5] + spb[y_step1o] + spb[ny_step1o]) >> 2;
			dpb[2] = (spb[2] + spb[6] + spb[y_step2o] + spb[ny_step2o]) >> 2;

			dpt[0] = (spt[0] + spt[-4] + spt[y_step] + spt[ny_step]) >> 2;
			dpt[1] = (spt[1] + spt[-3] + spt[y_step1o] + spt[ny_step1o]) >> 2;
			dpt[2] = (spt[2] + spt[-2] + spt[y_step2o] + spt[ny_step2o]) >> 2;
		}

		// Middle
		for (auto dp = UCHAR_PTR(surf.buffer + surf.size.x + 1), end = UCHAR_PTR(surf.end - surf.size.x),
			sp = UCHAR_PTR(cache_buffer.buffer + surf.size.x + 1); dp < end; dp += 8, sp += 8)
		{
			for (auto x_end = dp + y_step2n; dp < x_end; dp += 4, sp += 4)
			{
				dp[0] = (sp[0] + sp[4] + sp[-4] + sp[y_step] + sp[ny_step]) / 5;
				dp[1] = (sp[1] + sp[5] + sp[-3] + sp[y_step1o] + sp[ny_step1o]) / 5;
				dp[2] = (sp[2] + sp[6] + sp[-2] + sp[y_step2o] + sp[ny_step2o]) / 5;
			}
		}

		return false;
	}

	bool slight_blur(csr surf)
	{
		if (surf.buffer_size > CACHE_BUFFER_SIZE) return true;
		cache_buffer.end = cache_buffer.buffer + surf.buffer_size;

		surf >> cache_buffer;

		const int y_step = surf.size.x << 2, ny_step = -y_step,
			y_step1n = y_step - 4, y_step2n = y_step - 8,
			y_step1o = y_step | 1, y_step2o = y_step | 2,
			ny_step1o = ny_step | 1, ny_step2o = ny_step | 2;

		// Bottom-Left
		auto dp = UCHAR_PTR(surf.buffer);
		dp[0] = ((dp[0] << 2) + dp[4] + dp[y_step]) / 6;
		dp[1] = ((dp[1] << 2) + dp[5] + dp[y_step1o]) / 6;
		dp[2] = ((dp[2] << 2) + dp[6] + dp[y_step2o]) / 6;

		// Bottom-Right
		dp += y_step1n;
		dp[0] = ((dp[0] << 2) + dp[-4] + dp[y_step]) / 6;
		dp[1] = ((dp[1] << 2) + dp[-3] + dp[y_step1o]) / 6;
		dp[2] = ((dp[2] << 2) + dp[-2] + dp[y_step2o]) / 6;

		// Top-Right
		dp = UCHAR_PTR(surf.end - 1);
		dp[0] = ((dp[0] << 2) + dp[-4] + dp[ny_step]) / 6;
		dp[1] = ((dp[1] << 2) + dp[-3] + dp[ny_step1o]) / 6;
		dp[2] = ((dp[2] << 2) + dp[-2] + dp[ny_step2o]) / 6;

		// Top-Left
		dp -= y_step1n;
		dp[0] = ((dp[0] << 2) + dp[4] + dp[ny_step]) / 6;
		dp[1] = ((dp[1] << 2) + dp[5] + dp[ny_step1o]) / 6;
		dp[2] = ((dp[2] << 2) + dp[6] + dp[ny_step2o]) / 6;

		// Bottom & Top Middle
		for (auto dpb = UCHAR_PTR(surf.buffer + 1), dpt = UCHAR_PTR(surf.end) - y_step1n, end = dpb + y_step2n,
			spb = UCHAR_PTR(cache_buffer.buffer + 1), spt = UCHAR_PTR(cache_buffer.end) - y_step1n;
			dpb < end; dpb += 4, dpt += 4, spb += 4, spt += 4)
		{
			dpb[0] = ((spb[0] << 2) + spb[4] + spb[-4] + spb[y_step]) / 7;
			dpb[1] = ((spb[1] << 2) + spb[5] + spb[-3] + spb[y_step1o]) / 7;
			dpb[2] = ((spb[2] << 2) + spb[6] + spb[-2] + spb[y_step2o]) / 7;

			dpt[0] = ((spt[0] << 2) + spt[4] + spt[-4] + spt[ny_step]) / 7;
			dpt[1] = ((spt[1] << 2) + spt[5] + spt[-3] + spt[ny_step1o]) / 7;
			dpt[2] = ((spt[2] << 2) + spt[6] + spt[-2] + spt[ny_step2o]) / 7;
		}

		// Left & Right Middle
		for (auto dpb = UCHAR_PTR(surf.buffer + surf.size.x), dpt = dpb + y_step1n, end = UCHAR_PTR(surf.end - surf.size.x),
			spb = UCHAR_PTR(cache_buffer.buffer + surf.size.x), spt = spb + y_step1n;
			dpb < end; dpb += y_step, dpt += y_step, spb += y_step, spt += y_step)
		{
			dpb[0] = ((spb[0] << 2) + spb[4] + spb[y_step] + spb[ny_step]) / 7;
			dpb[1] = ((spb[1] << 2) + spb[5] + spb[y_step1o] + spb[ny_step1o]) / 7;
			dpb[2] = ((spb[2] << 2) + spb[6] + spb[y_step2o] + spb[ny_step2o]) / 7;

			dpt[0] = ((spt[0] << 2) + spt[-4] + spt[y_step] + spt[ny_step]) / 7;
			dpt[1] = ((spt[1] << 2) + spt[-3] + spt[y_step1o] + spt[ny_step1o]) / 7;
			dpt[2] = ((spt[2] << 2) + spt[-2] + spt[y_step2o] + spt[ny_step2o]) / 7;
		}

		// Middle
		for (auto dp = UCHAR_PTR(surf.buffer + surf.size.x + 1), end = UCHAR_PTR(surf.end - surf.size.x),
			sp = UCHAR_PTR(cache_buffer.buffer + surf.size.x + 1); dp < end; dp += 8, sp += 8)
		{
			for (auto x_end = dp + y_step2n; dp < x_end; dp += 4, sp += 4)
			{
				dp[0] = ((sp[0] << 2) + sp[4] + sp[-4] + sp[y_step] + sp[ny_step]) >> 3;
				dp[1] = ((sp[1] << 2) + sp[5] + sp[-3] + sp[y_step1o] + sp[ny_step1o]) >> 3;
				dp[2] = ((sp[2] << 2) + sp[6] + sp[-2] + sp[y_step2o] + sp[ny_step2o]) >> 3;
			}
		}

		return false;
	}

	inline bool is_inside(point pos, point start, point end)
	{
		return (
			pos.x >= start.x
			&& pos.x < end.x
			&& pos.y >= start.y
			&& pos.y < end.y
			);
	}

	inline bool is_inside_size(point pos, point start, point size)
	{
		return (
			pos.x >= start.x
			&& pos.y >= start.y
			&& pos.x < start.x + size.x
			&& pos.y < start.y + size.y
			);
	}

	inline bool is_inside(point pos, point lim)
	{
		return (
			pos.x >= 0
			&& pos.y >= 0
			&& pos.x < lim.x
			&& pos.y < lim.y
			);
	}

	inline color_t* get_raw_pixel(point pos, csr surf) { return &surf.buffer[pos.x + pos.y * surf.size.x]; }
	inline color_t get_raw_color(point pos, csr surf) { return surf.buffer[pos.x + pos.y * surf.size.x]; }

	inline color_t* get_pixel(point pos, csr surf)
	{
		return (
			pos.x >= 0
			&& pos.y >= 0
			&& pos.x < surf.size.x
			&& pos.y < surf.size.y
			) ? &surf.buffer[pos.x + pos.y * surf.size.x] : nullptr;
	}

	inline color_t get_color(point pos, csr surf)
	{
		return (
			pos.x >= 0
			&& pos.y >= 0
			&& pos.x < surf.size.x
			&& pos.y < surf.size.y
			) ? surf.buffer[pos.x + pos.y * surf.size.x] : C_black;
	}

	inline void set_sure_pixel(point pos, color_t color, csr surf)
	{
		surf.buffer[pos.x + pos.y * surf.size.x] = color;
	}

	inline void set_pixel(point pos, color_t color, csr surf)
	{
		if (pos.x >= 0
			&& pos.y >= 0
			&& pos.x < surf.size.x
			&& pos.y < surf.size.y
			) surf.buffer[pos.x + pos.y * surf.size.x] = color;
	}

	inline bool straighten_line(point start, point& end)
	{
		if (abs(end.y - start.y) > abs(end.x - start.x))
		{
			end.x = start.x;
			return true;
		}
		end.y = start.y;
		return false;
	}

	void resize_surface(csr src, csr dest)
	{
		if (src.size == dest.size) return src >> dest;

		auto [mx, my] = src.size / dest.size;
		auto [evx, evy] = src.size % dest.size;
		my = (my - 1) * src.size.x;

		int errx, erry = evy;
		const color_t* sp = src.buffer;

		for (uint* dp = dest.buffer; dp < dest.end; sp += my, erry += evy)
		{
			errx = evx;
			for (const color_t* x_end = dp + dest.size.x; dp < x_end; ++dp, errx += evx, sp += mx)
			{
				*dp = *sp;
				if (errx >= dest.size.x)
				{
					++sp;
					errx -= dest.size.x;
				}
			}

			if (erry >= dest.size.y)
			{
				sp += src.size.x;
				erry -= dest.size.y;
			}
		}
	}

	constexpr option_t BLIT_SIGNATURE = 0b10;

	void _blit_cut_surface(csr bs, csr ss, point pos, uchar alpha, bool check)
	{
		point start = pos, size = ss.size;

		if (check)
		{
			size += pos;
			clamp_to_surface(start, bs);
			clamp_to_surface(size, bs);
			if (start ^= size) return;
			size -= start;
		}

		color_t* bs_p = get_raw_pixel(start, bs), * ss_p = get_raw_pixel(start - pos, ss);

		const int ysb = bs.size.x - size.x, yss = ss.size.x - size.x;

		color_t*& src = alpha ? ss_p : bs_p;
		color_t*& dest = alpha ? bs_p : ss_p;

		if (alpha & 0b1)
		{
			for (const color_t* ss_end = ss_p + ss.size.x * size.y; ss_p < ss_end; ss_p += yss, bs_p += ysb)
				for (const color_t* ss_x_end = ss_p + size.x; ss_p < ss_x_end; ++ss_p, ++bs_p)
					if (*src != C_transparent) *dest = *src;
			return;
		}

		for (const color_t* ss_end = ss_p + ss.size.x * size.y; ss_p < ss_end; ss_p += yss, bs_p += ysb)
			for (const color_t* ss_x_end = ss_p + size.x; ss_p < ss_x_end; ++ss_p, ++bs_p)
				*dest = *src;
	}

	inline void blit_surface(csr dest, csr src, point pos, bool alpha = false, bool check = true)
	{
		_blit_cut_surface(dest, src, pos, alpha | BLIT_SIGNATURE, check);
	}

	inline void cut_surface(csr dest, csr src, point pos, bool check = true)
	{
		_blit_cut_surface(src, dest, pos, false, check);
	}

	namespace draw
	{
		void fill_rect(point start, point end, color_t color, csr surf, uchar alpha = UINT8_MAX)
		{
			clamp_to_surface(start, surf);
			clamp_to_surface(end, surf);

			if (start ^= end) return;

			start | end;

			point size = end - start;
			uint* px = get_raw_pixel(start, surf);
			int y_step = surf.size.x - size.x;

			if (alpha == UINT8_MAX)
			{
				for (const color_t* y_end = px + size.y * surf.size.x; px < y_end; px += y_step)
					for (const color_t* x_end = px + size.x; px < x_end; ++px)
						*px = color;
				return;
			}

			y_step <<= 2;
			size <<= 2;

			auto px_ptr = UCHAR_PTR(px);
			auto color_ptr = UCHAR_PTR(&color);
			uchar b = color_ptr[0],
				g = color_ptr[1],
				r = color_ptr[2];

			for (const uchar* y_end = px_ptr + size.y * surf.size.x; px_ptr < y_end; px_ptr += y_step)
				for (const uchar* x_end = px_ptr + size.x; px_ptr < x_end; px_ptr += 4)
				{
					px_ptr[0] = slide_uchar(px_ptr[0], b, alpha);
					px_ptr[1] = slide_uchar(px_ptr[1], g, alpha);
					px_ptr[2] = slide_uchar(px_ptr[2], r, alpha);
				}
		}

		void _straight_line(int d1, int d2, int s, bool slope, color_t color, csr surf, int dash = 0, int thickness = 1)
		{
			if (s < 0 || d1 == d2) return;

			if (thickness > 1)
			{
				int ts = thickness >> 1, te = ts + (thickness & 1);
				return slope
					? fill_rect({ s - te, d1 }, { s + ts, d2 }, color, surf)
					: fill_rect({ d1, s - te }, { d2, s + ts }, color, surf);
			}

			const point size = slope ? ~surf.size : surf.size;

			if (s >= surf.size.x) return;
			if (d1 > d2) std::swap(d1, d2);

			int cache = d1;
			d1 = std::clamp(d1, 0, size.x);
			d2 = std::clamp(d2, 0, size.x);

			if (d1 == d2) return;

			point steps = slope ? point{ surf.size.x, 1 } : point{ 1, surf.size.x };

			color_t* px = surf.buffer + s * steps.y;
			color_t* const end = px + d2 * steps.x;
			px += d1 * steps.x;

			if (dash)
			{
				const int d_dash = dash << 1;
				cache = (d1 - cache) % d_dash;
				int index = 1;
				const int dash_step = dash * steps.x;

				if (cache >= dash)
					px += (d_dash - cache) * steps.x;
				else
					index = cache + 1;

				while (px < end)
				{
					*px = color;
					px += steps.x;
					if (index == dash)
					{
						index = 1;
						px += dash_step;
					}
					else ++index;
				}

				return;
			}

			while (px < end)
			{
				*px = color;
				px += steps.x;
			}
		}

		inline void straight_line(point start, point end, color_t color, csr surf, int dash = 0, int thickness = 1)
		{
			if (end.x == start.x)
				_straight_line(start.y, end.y, start.x, true, color, surf, dash, thickness);
			else if (end.y == start.y)
				_straight_line(start.x, end.x, start.y, false, color, surf, dash, thickness);
		}

		void line(point start, point end, color_t color, csr surf)
		{
			point delta = end - start;

			if (delta.y == 0)
			{
				if (delta.x != 0) _straight_line(start.x, end.x, start.y, false, color, surf);
				return;
			}

			if (delta.x == 0)
			{
				if (delta.y != 0) _straight_line(start.y, end.y, start.x, true, color, surf);
				return;
			}

			bool slope = abs(delta.y) > abs(delta.x);

			if (slope)
			{
				std::swap(start.x, start.y);
				std::swap(end.x, end.y);
			}

			if (start.x > end.x)
			{
				std::swap(start.x, end.x);
				std::swap(start.y, end.y);
			}

			delta = end - start;
			int error = delta.x >> 1, abs_delta_y = abs(delta.y);
			point step = { 1, get_sign(delta.y) };
			delta.y = abs(delta.y);
			static color_t* px;

			if (slope)
			{
				// reverse get_raw_pixel(p, s);
				px = &surf.buffer[start.y + start.x * surf.size.x];
				step.x = surf.size.x;
			}
			else
			{
				px = get_raw_pixel(start, surf);
				step.y *= surf.size.x;
			}

			for (int i = start.x; i < end.x; ++i, px += step.x)
			{
				*px = color;
				error -= abs_delta_y;

				if (error < 0)
				{
					px += step.y;
					error += delta.x;
				}
			}
		}

		void circle(point center, int radius, color_t color, csr surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				set_pixel({ center.x - diff.x, center.y - diff.y }, color, surf);
				set_pixel({ center.x - diff.x, center.y + diff.y }, color, surf);
				set_pixel({ center.x + diff.x, center.y - diff.y }, color, surf);
				set_pixel({ center.x + diff.x, center.y + diff.y }, color, surf);

				set_pixel({ center.x - diff.y, center.y - diff.x }, color, surf);
				set_pixel({ center.x - diff.y, center.y + diff.x }, color, surf);
				set_pixel({ center.x + diff.y, center.y - diff.x }, color, surf);
				set_pixel({ center.x + diff.y, center.y + diff.x }, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		void sure_circle(point center, int radius, color_t color, csr surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				set_sure_pixel({ center.x - diff.x, center.y - diff.y }, color, surf);
				set_sure_pixel({ center.x - diff.x, center.y + diff.y }, color, surf);
				set_sure_pixel({ center.x + diff.x, center.y - diff.y }, color, surf);
				set_sure_pixel({ center.x + diff.x, center.y + diff.y }, color, surf);

				set_sure_pixel({ center.x - diff.y, center.y - diff.x }, color, surf);
				set_sure_pixel({ center.x - diff.y, center.y + diff.x }, color, surf);
				set_sure_pixel({ center.x + diff.y, center.y - diff.x }, color, surf);
				set_sure_pixel({ center.x + diff.y, center.y + diff.x }, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		inline void sure_basic_line_x(int xs, int xb, int y, color_t color, csr surf)
		{
			color_t* px = surf.buffer + surf.size.x * y;
			color_t* const end = px + xb;
			px += xs;
			while (px < end)
			{
				*px = color;
				++px;
			}
		}

		inline void sure_basic_line_y(int ys, int yb, int x, color_t color, csr surf)
		{
			color_t* px = surf.buffer + x; // 4
			color_t* const end = px + yb * surf.size.x; // 7
			px += ys * surf.size.x; // 7
			while (px < end)
			{
				*px = color;
				px += surf.size.x;
			}
		}

		void sure_fill_circle(point center, int radius, color_t color, csr surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				sure_basic_line_x(center.x - diff.x, center.x + diff.x, center.y + diff.y, color, surf);
				sure_basic_line_x(center.x - diff.x, center.x + diff.x, center.y - diff.y, color, surf);
				sure_basic_line_x(center.x - diff.y, center.x + diff.y, center.y + diff.x, color, surf);
				sure_basic_line_x(center.x - diff.y, center.x + diff.y, center.y - diff.x, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		void circle(point center, int inner, int outer, color_t color, csr surf)
		{
			int xo = outer, xi = inner, y = 0, erro = 1 - xo, erri = 1 - xi;

			while (xo >= y)
			{
				_straight_line(center.x - xo, center.x - xi, center.y - y, false, color, surf);
				_straight_line(center.x - xo, center.y - xi, center.x - y, color, true, surf);
				_straight_line(center.x - xo, center.x - xi, center.y + y, false, color, surf);
				_straight_line(center.x - xo, center.y - xi, center.x + y, color, true, surf);

				_straight_line(center.x + xi, center.x + xo, center.y - y, false, color, surf);
				_straight_line(center.x + xi, center.y + xo, center.x - y, color, true, surf);
				_straight_line(center.x + xi, center.x + xo, center.y + y, false, color, surf);
				_straight_line(center.x + xi, center.y + xo, center.x + y, color, true, surf);

				if (erro < 0)
					erro += (++y << 1) + 1;
				else
					erro += 2 * (++y - --xo + 1);

				if (y > inner)
					xi = y;
				else if (erri < 0)
					erri += (y << 1) + 1;
				else
					erri += 2 * (y - --xi + 1);
			}
		}

		inline void circle(point center, int radius, int thickness, bool inner, color_t color, csr surf)
		{
			if (inner)
				circle(center, radius - thickness, radius, color, surf);
			else
				circle(center, radius, radius + thickness, color, surf);
		}

		inline void rect(point start, point end, color_t color, csr surf, int dash = 0, int thickness = 1)
		{
			_straight_line(start.x, end.x, start.y, false, color, surf, dash, thickness);
			_straight_line(start.x, end.x, end.y, false, color, surf, dash, thickness);
			_straight_line(start.y, end.y, start.x, true, color, surf, dash, thickness);
			_straight_line(start.y, end.y, end.x, true, color, surf, dash, thickness);
		}

		inline void rect(point start, point end, bool b, bool t, bool l, bool r, color_t color, csr surf, int dash = 0, int thickness = 1)
		{
			start | end;
			if (b) _straight_line(start.x, end.x, start.y, false, color, surf, dash, thickness);
			if (t) _straight_line(start.x, end.x, end.y, false, color, surf, dash, thickness);
			if (l) _straight_line(start.y, end.y, start.x, true, color, surf, dash, thickness);
			if (r) _straight_line(start.y, end.y, end.x, true, color, surf, dash, thickness);
		}

		inline void triangle(point a, point b, point c, color_t color, csr surf)
		{
			line(a, b, color, surf);
			line(b, c, color, surf);
			line(c, a, color, surf);
		}
	}
}