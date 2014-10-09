#pragma once

#include "Stream.h"

namespace util {

	//////////////////////////////////////////////////////////////////////////
	// Supports Utf8 and Utf16, little and big endian.
	//////////////////////////////////////////////////////////////////////////

	namespace textfile {
		enum Format { utf8, utf8noBom, utf16, utf16rev };
	}
}

#include "TextReader.h"
#include "TextWriter.h"
