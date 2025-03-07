#pragma once

#include <ostream>
#include <algorithm>

struct point
{
	int x, y;
};

inline point P(int x, int y) { return { x, y }; }

inline point operator -(point p) { return { -p.x, -p.y }; }
inline point operator ~(point p) { return { p.y, p.x }; }

inline bool operator ^=(point a, point b) { return a.x == b.x || a.y == b.y; }

inline void swap_point_if(point& s, point& b)
{
	if (s.x > b.x) std::swap(s.x, b.x);
	if (s.y > b.y) std::swap(s.y, b.y);
}

inline void operator |(point& s, point& b) { swap_point_if(s, b); }

inline void clamp_point(point& p, point s, point b)
{
	p.x = std::clamp(p.x, s.x, b.x);
	p.y = std::clamp(p.y, s.y, b.y);
}

inline std::ostream& operator <<(std::ostream& os, point p)
{
	os << '(' << p.x << ',' << ' ' << p.y << ')';
	return os;
}

#define OPA(S) inline point operator S(point a, point b) { return { a.x S b.x, a.y S b.y }; }
#define OIA(S) inline point operator S(point a, int b) { return { a.x S b, a.y S b }; }
#define OB(S, J) inline bool operator S(point a, point b) { return  a.x S b.x J a.y S b.y; }
#define OPV(S) inline void operator S(point& a, point b) { a.x S b.x; a.y S b.y; }
#define OIV(S) inline void operator S(point& a, int b) { a.x S b; a.y S b; }

OPA(+)
OPA(-)
OPA(*)
OPA(/)
OPA(%)

OIA(+)
OIA(-)
OIA(*)
OIA(/)
OIA(%)
OIA(&)
OIA(|)
OIA(<<)
OIA(>>)

OB(==, &&)
OB(!=, ||)
OB(<, &&)
OB(>, &&)
OB(<=, &&)
OB(>=, &&)

OPV(+=)
OPV(-=)

OIV(+=)
OIV(-=)
OIV(<<=)
OIV(>>=)
