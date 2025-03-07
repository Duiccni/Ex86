#pragma once

#define CONSOLE 1
#define SUCCESS 0

#if CONSOLE == 1
#include <iostream>
#endif

#include <fstream>
#include <algorithm>
#include "Point.h"
#include "Colors.h"

#define FLOAT1DIV60 0.016666666666666666f

#define MODULO(x, a) ((((x) % (a)) + (a)) % (a))
#define SIZE_MUL(a, b) static_cast<size_t>(a) * (b)
#define UCHAR_PTR(a) reinterpret_cast<uchar*>(a)
#define TYPE_MALLOC(T, size) static_cast<T*>(malloc((size) * sizeof(T)))

using uint = unsigned int;
using uchar = unsigned char;
using option_t = unsigned char;

inline uchar slide_uchar(uchar x1, uchar x2, uchar t)
{
	return x1 + (x2 - x1) * t / UINT8_MAX;
}

inline int get_sign(int x)
{
	return (x >> 31) | 1;
}
