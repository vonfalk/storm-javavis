#pragma once
#include "Named.h"

namespace storm {
	STORM_PKG(core.info);

	/**
	 * Holds license information about a package. Each instance holds a single licence.
	 */
	class License : public Named {
		STORM_CLASS;
	public:
		// Create. 'id' is the identifier in the name tree.
		STORM_CTOR License(Str *id, Str *title, Str *body);

		// License title.
		Str *title;

		// License body.
		Str *body;

		// To string.
		void STORM_FN toS(StrBuf *to) const;
	};


	// Find all licenses in a part of the name tree.
	Array<License *> *STORM_FN licenses(EnginePtr e) ON(Compiler);
	Array<License *> *STORM_FN licenses(Named *root) ON(Compiler);

}
