#include "stdafx.h"
#include "Image.h"
#include "ImageLoad.h"

namespace graphics {

	namespace wic {
		Image *loadImage(storm::IStream *from, const wchar *&error);
	}


	// check the header in the stream
	// zeroTerm - Zero-terminated string in the file?
	bool checkHeader(storm::IStream *file, const char *header, bool zeroTerm) {
		nat len = strlen(header);
		if (zeroTerm)
			len++;

		Buffer buffer = file->peek(storm::buffer(file->engine(), len));
		if (!buffer.full()) {
			return false;
		}

		for (nat i = 0; i < len; i++) {
			if (byte(buffer[i]) != byte(header[i])) {
				return false;
			}
		}

		return true;
	}

	Image *loadImage(Url *file) {
		return loadImage(file->read());
	}

	Image *loadImage(storm::IStream *from) {
		Image *loaded = null;
		const wchar *error = S("The image file type was not recognized.");

		if (checkHeader(from, "\xff\xd8", false)) {
			// JPEG file
			loaded = loadJpeg(from, error);
		} else if (checkHeader(from, "JFIF", false)) {
			// JFIF file
			loaded = loadJpeg(from, error);
		} else if (checkHeader(from, "Exif", false)) {
			// Exif file
			loaded = loadJpeg(from, error);
		} else if (checkHeader(from, "\x89PNG", false)) {
			// PNG file
			loaded = loadPng(from, error);
		} else if (checkHeader(from, "BM", false)) {
			// BMP file
			loaded = loadBmp(from, error);
		}

		if (!loaded)
			throw ImageLoadError(error);

		return loaded;
	}
}
