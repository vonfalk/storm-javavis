#include "StdAfx.h"
#include "Font.h"

Font::Font() :
familyName(L""),
size(0.0f),
weight(0),
style(fsNormal),
sizeMode(smCharHeight),
stretch(0) {}

Font::Font(const String &familyName, float size) :
familyName(familyName),
size(size),
weight(400),
style(fsNormal),
sizeMode(smCharHeight),
stretch(5) {}

bool Font::operator ==(const Font &o) const {
	if (familyName != o.familyName) return false;
	if (weight != o.weight) return false;
	if (style != o.style) return false;
	if (stretch != o.stretch) return false;
	if (size != o.size) return false;
	return true;
}

Font Font::defaultFont() {
	static bool initialized = false;
	static Font font;

	if (!initialized) {
		initialized = true;

		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);

		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		LOGFONT &lf = ncm.lfMessageFont;
		font.familyName = lf.lfFaceName;
		if (lf.lfHeight < 0) {
			font.sizeMode = smCharHeight;
		} else {
			font.sizeMode = smCellHeight;
		}
		font.size = (float)abs(lf.lfHeight);
		font.weight = lf.lfWeight;
		font.style = fsNormal;
		font.stretch = 5;
	}

	// return Font(L"Segoe UI", 14.0f);
	return font;
}

Font Font::defaultCodeFont() {
	static Font font(L"Courier New", 14.0f);
	return font;
}

