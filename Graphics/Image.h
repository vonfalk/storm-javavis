#pragma once
#include "Shared/Io/Stream.h"
#include "Shared/Io/Url.h"
#include "Utils/Exception.h"

namespace graphics {

	/**
	 * Loading images. Supports various formats, auto-detects the format based on header magic numbers.
	 * Currently supported:
	 * - PNG
	 * - JPG
	 * - BMP
	 */
	Image *STORM_FN loadImage(Par<Url> file);
	Image *STORM_FN loadImage(Par<IStream> from);


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
