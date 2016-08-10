#pragma once
#include "NameSet.h"
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Defines the contents of a package. A package is a NameSet associated with a path, which
	 * allows it to load things from there.
	 */
	class Package : public NameSet {
		STORM_CLASS;
	public:
		// Create a virtual package, ie. a package that is not present on disk. These packages must
		// be eagerly loaded by 'add'ing things to them.
		STORM_CTOR Package(Str *name);

		// Create a package located in 'path'.
		STORM_CTOR Package(Url *path);

		// Get parent.
		virtual NameLookup *STORM_FN parent() const;

		// Get our url.
		virtual MAYBE(Url *) STORM_FN url() const;

		// Lazy-loading.
		virtual Bool STORM_FN loadName(SimplePart *part);
		virtual Bool STORM_FN loadAll();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Our path. Points tu null if we're a virtual package.
		Url *pkgPath;

		/**
		 * Loading of sub-packages.
		 */

		// Load all files given.
		void loadFiles(Array<Url *> *files);

		// Try to load a sub-package. Returns null on failure.
		Package *loadPackage(Str *name);
	};

}
