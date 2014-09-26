#pragma once

class Font {
public:
	// Create an empty font.
	Font();

	// Create a default font based on name and size.
	Font(const String &familyName, float size);

	// Font style enumeration
	enum FontStyle { fsNormal, fsOblique, fsItalic };

	// The family name of the font.
	String familyName;

	// The weight of the font. An integer between 1 and 999,
	// where 1 is thin and 999 is bold. Normal is at 400.
	nat weight;

	// The style of the font
	FontStyle style;

	// The stretch of the font, where 1 is narrow and 9 is wide.
	// The default value is 5.
	nat stretch;

	// The size of the font in device independent pixels, 1/96 inch.
	float size;

	// The mode of the size parameter.
	// See documentation on "lfHeight" for "LOGFONT" for explanation.
	enum SizeMode { smCellHeight, smCharHeight };
	SizeMode sizeMode;

	// Equality operators
	bool operator ==(const Font &o) const;
	inline bool operator !=(const Font &o) const { return !(*this == o); }

	// Create some default fonts.
	static Font defaultFont();
	static Font defaultCodeFont();
};
