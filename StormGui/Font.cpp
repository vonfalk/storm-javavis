#include "stdafx.h"
#include "Font.h"

namespace stormgui {

	Font::Font(HFONT font) : font(font) {}

	HFONT Font::handle() {
		return font;
	}

	Font *defaultFont(EnginePtr e) {
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		HFONT font = CreateFontIndirect(&ncm.lfMessageFont);
		return CREATE(Font, e.v, font);
	}

}
