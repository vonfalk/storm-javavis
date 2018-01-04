#pragma once
#include "Core/Graphics/Image.h"
#include "Core/Io/Stream.h"

namespace graphics {

	Image *loadBmp(IStream *from, const wchar *&error);

}
