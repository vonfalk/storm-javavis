#pragma once

#include <Gdiplus.h>

class Vector;

namespace util {
	class Stream;
}

// rgba color used within directx
// DO NOT CHANGE LAYOUT OR SIZE OF THIS OBJECT. IT IS ASSUMED TO BE EXACTLY AS DirectX WANTS IT.
class Color {
public:
	// initialize to black
	Color();

	// create from parameters
	Color(float r, float g, float b, float a = 1.0f);

	// create from regular color, setting alpha to 1.0f
	Color(COLORREF color, float a = 1.0f);

	// create from a byte array containing RGBA colors (see TexturePlane.h)
	Color(const byte *src);

	operator Gdiplus::Color() const { return Gdiplus::Color(BYTE(255.0f * a), BYTE(255.0f * r), BYTE(255.0f * g), BYTE(255.0f * b)); }

	// Get a system color.
	static Color sysColor(int id);

	// save as RGBA
	void save(byte *to) const;

	// Return as a COLORREF
	COLORREF ref() const;

	// Save/load interface.
	static Color load(util::Stream &from);
	void save(util::Stream &to) const;
	

	float r, g, b, a;

	
	// operators
	inline Color operator +(const Color &other) const { return Color(r + other.r, g + other.g, b + other.b, a + other.a); }
	inline Color operator -(const Color &other) const { return Color(r - other.r, g - other.g, b - other.b, a - other.a); }
	inline Color operator *(float c) const { return Color(r * c, g * c, b * c, a * c); }
	inline Color operator /(float c) const { return *this * (1.0f / c); }

	inline Color &operator +=(const Color &other) { r += other.r; g += other.g; b += other.b; a += other.a; return *this; }
	inline Color &operator -=(const Color &other) { r -= other.r; g -= other.g; b -= other.b; a -= other.a; return *this; }

	inline bool operator ==(const Color &other) const { return r == other.r && g == other.g && b == other.b; }
	inline bool operator !=(const Color &other) const { return !(*this == other); }

	// convenient in some cases
	Color &operator =(const Vector &pt);

	// return the DX-compatible color (this one actually is compatible...)
	inline const float *toDx() const { return &r; }


	// predefined colors
	inline static Color black() { return Color(0, 0, 0); }
	inline static Color white() { return Color(1, 1, 1); }
	inline static Color red() { return Color(1, 0, 0); }
	inline static Color green() { return Color(0, 1, 0); }
	inline static Color blue() { return Color(0, 0, 1); }
	inline static Color ltBlue() { return Color(0.3f, 0.3f, 1.0f); }
};

std::wostream &operator <<(std::wostream &to, const Color &c);
