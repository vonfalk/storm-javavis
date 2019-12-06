#pragma once
#include "Named.h"
#include "Reader.h"

namespace storm {
	STORM_PKG(core.info);

	/**
	 * Holds license information about a package. Each instance holds a single licence.
	 */
	class License : public Named {
		STORM_CLASS;
	public:
		// Create. 'id' is the identifier in the name tree.
		STORM_CTOR License(Str *id, Str *title, Str *author, Str *body);

		// License title.
		Str *title;

		// License author.
		Str *author;

		// License body.
		Str *body;

		// To string.
		void STORM_FN toS(StrBuf *to) const;
	};


	// Find all licenses in a part of the name tree.
	Array<License *> *STORM_FN licenses(EnginePtr e) ON(Compiler);
	Array<License *> *STORM_FN licenses(Named *root) ON(Compiler);


	STORM_PKG(lang.license);

	namespace license {
		// Reader for licenses.
		PkgReader *STORM_FN reader(Array<Url *> *files, Package *pkg) ON(Compiler);
	}

	/**
	 * Reader for license files.
	 *
	 * We don't bother implementing individual file readers, as syntax highlighting is not very
	 * important for this file type.
	 */
	class LicenseReader : public PkgReader {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR LicenseReader(Array<Url *> *files, Package *pkg);

		// We read licenses as types.
		virtual void STORM_FN readTypes();

	private:
		// Load a single license.
		License *STORM_FN readLicense(Url *file);
	};
}
