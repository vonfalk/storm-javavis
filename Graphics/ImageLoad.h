#pragma once
#include "Image.h"

namespace graphics {

	/**
	 * Loading specific image formats. Not exported to Storm. Only the generic 'loadImage' is
	 * needed. These may return null on failure.
	 */
	Image *loadPng(IStream *from, const wchar *&error);
	Image *loadJpeg(IStream *from, const wchar *&error);
	Image *loadBmp(IStream *from, const wchar *&error);

}
