#include "stdafx.h"
#include "Read.h"
#include "OggSound.h"
#include "Exception.h"

namespace sound {

	// check the header in the stream
	// zeroTerm - Zero-terminated string in the file?
	static bool checkHeader(IStream *file, char *header, bool zeroTerm) {
		nat len = strlen(header);
		if (zeroTerm)
			len++;

		storm::Buffer buffer = file->peek(storm::buffer(file->engine(), len));
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


	Sound *sound(IStream *src) {
		Sound *result = null;

		if (checkHeader(src, "OggS", false)) {
			result = openOgg(src->randomAccess());
		} else {
			throw SoundOpenError(L"Unknown file format.");
		}

		if (!result)
			throw SoundOpenError(L"Failed to open file.");

		return result;
	}

	Sound *soundStream(IStream *src) {
		Sound *result = null;

		if (checkHeader(src, "OggS", false)) {
			result = openOggStream(src);
		} else {
			throw SoundOpenError(L"Unknown file format.");
		}

		if (!result)
			throw SoundOpenError(L"Failed to open file.");

		return result;
	}

}
