#pragma once
#include "Doc.h"
#include "Core/Gen/CppTypes.h"
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Documentation for functions implemented in C++. Provided by the CppLoader class.
	 */
	class CppDoc : public NamedDoc {
		STORM_CLASS;
	public:
		// Create.
		CppDoc(Named *entity, Url *file, Nat entry, MAYBE(const CppParam *) params);

		// Create with a simple name instead of an Url. The name is assumed to be relative the root package's Url.
		CppDoc(Named *entity, const wchar *file, Nat entry, MAYBE(const CppParam *) params);

		// Get documentation.
		virtual Doc *STORM_FN get();

	private:
		// Original entity.
		Named *entity;

		// Url of the file containing documentation or a simple c-string containing the file name.
		UNKNOWN(PTR_GC) const void *data;

		// Pointer to the CppParams entry for this entity so that we can retrieve the parameter names.
		MAYBE(const CppParam *) params;

		// Documentation entry in the file and data indicating if 'data' is a c-string or an URL (least significant bit).
		Nat entryInfo;

		// Get an URL to the file.
		Url *file();

		// Get the entry.
		Nat entry();
	};

}
