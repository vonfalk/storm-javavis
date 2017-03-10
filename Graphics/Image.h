#pragma once
#include "Core/Io/Stream.h"
#include "Core/Io/Url.h"
#include "Core/Graphics/Image.h"
#include "Utils/Exception.h"

namespace graphics {

	/**
	 * Loading images. Supports various formats, auto-detects the format based on header magic numbers.
	 * Currently supported:
	 * - PNG
	 * - JPG
	 * - BMP
	 */
	Image *STORM_FN loadImage(Url *file);
	Image *STORM_FN loadImage(IStream *from);

	/**
	 * Error.
	 */
	class ImageLoadError : public Exception {
	public:
		ImageLoadError(const String &msg) : msg(msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};

}
