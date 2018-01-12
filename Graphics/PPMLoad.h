#pragma once
#include "Core/Io/Stream.h"
#include "Core/Graphics/Image.h"

namespace graphics {

	// Is this file a PPM file?
	bool isPPM(IStream *file);

	// Load a PPM file.
	Image *loadPPM(IStream *from, const wchar *&error);

}
