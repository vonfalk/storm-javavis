#pragma once

class TextFormat {
public:
	// Horizontal text alignment.
	enum HAlign {
		hLeft, hCenter, hRight
	};
	HAlign hAlignment;

	// Vertical text alignment.
	enum VAlign {
		vTop, vCenter, vBottom
	};
	VAlign vAlignment;

	inline TextFormat() : hAlignment(hLeft), vAlignment(vTop) {}
	inline TextFormat(HAlign h, VAlign v) : hAlignment(h), vAlignment(v) {}

	inline static TextFormat centered() {
		return TextFormat(hCenter, vCenter);
	}

	// Equality operators
	inline bool operator ==(const TextFormat &o) const {
		if (hAlignment != o.hAlignment) return false;
		if (vAlignment != o.vAlignment) return false;
		return true;
	}
	inline bool operator !=(const TextFormat &o) const { return !(*this == o); }
};
