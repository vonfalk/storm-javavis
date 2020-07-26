#pragma once
#include "Core/Io/Url.h"
#include "Core/Io/Buffer.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * An Url that specifies the contents of a particular file.
	 *
	 * Thus, calling 'read' will return a reader for the pre-defined data rather than the file
	 * system. Aside from that, the Url will behave as the original url. This class is therefore
	 * usable for mocking file system content for various parts of the system.
	 */
	class UrlWithContents : public Url {
		STORM_CLASS;
	public:
		// Create with binary data.
		STORM_CTOR UrlWithContents(Url *original, Buffer data);

		// Create with string data.
		STORM_CTOR UrlWithContents(Url *original, Str *data);

		// Custom implementation of 'read'.
		virtual IStream *STORM_FN read();

	private:
		// Data.
		Buffer data;
	};

}
