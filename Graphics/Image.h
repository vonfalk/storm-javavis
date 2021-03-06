#pragma once
#include "Core/Io/Stream.h"
#include "Core/Io/Url.h"
#include "Core/Graphics/Image.h"
#include "Core/Exception.h"

namespace graphics {

	/**
	 * Loading images. Supports various formats, auto-detects the format based on header magic numbers.
	 * Currently supported:
	 * - PNG
	 * - JPG
	 * - BMP
	 * - PPM
	 */
	Image *STORM_FN loadImage(Url *file);
	Image *STORM_FN loadImage(IStream *from);

	/**
	 * Error.
	 */
	class EXCEPTION_EXPORT ImageLoadError : public storm::Exception {
		STORM_EXCEPTION;
	public:
		ImageLoadError(const wchar *msg) {
			this->msg = new (this) Str(msg);
		}
		STORM_CTOR ImageLoadError(Str *msg) {
			this->msg = msg;
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};

}
