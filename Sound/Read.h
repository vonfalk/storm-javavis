#pragma once
#include "Core/Io/Stream.h"
#include "Sound.h"

namespace sound {

	/**
	 * Read sound files. Try to figure out what format is contained by examining the header.
	 */

	// Open a sound file.
	Sound *STORM_FN sound(IStream *src) ON(Audio);

	// Open sound from a streaming source. The resulting stream will not be seekable.
	Sound *STORM_FN soundStream(IStream *src) ON(Audio);

}
