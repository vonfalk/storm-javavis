#pragma once
#include "Core/Char.h"
#include "Code/RefSource.h"
#include "Code/OffsetSource.h"

namespace storm {
	STORM_PKG(core.asm);

	class Named;

	/**
	 * A RefSource that refers to a named entity inside the compiler. Supports disambiguating
	 * mutliple sources within a single entity by optionally appending a single character to the
	 * end of the string.
	 */
	class NamedSource : public code::RefSource {
		STORM_CLASS;
	public:
		STORM_CTOR NamedSource(Named *entity);
		STORM_CTOR NamedSource(Named *entity, Char subtype);

		// Get the title.
		virtual Str *STORM_FN title() const;

		// Get the named entity referred to.
		Named *STORM_FN named() const { return entity; }

		// Get the subtype.
		Char STORM_FN type() const { return subtype; }

	private:
		// Entity we're referring to.
		Named *entity;

		// Subtype, if any. Char(0) indicates no subtype.
		Char subtype;
	};


	/**
	 * A named RefSource for offsets. Works like NamedSource.
	 */
	class NamedOffsetSource : public code::OffsetSource {
		STORM_CLASS;
	public:
		STORM_CTOR NamedOffsetSource(Named *entity);
		STORM_CTOR NamedOffsetSource(Named *entity, Char subtype);

		// Get the title.
		virtual Str *STORM_FN title() const;

		// Get the named entity referred to.
		Named *STORM_FN named() const { return entity; }

		// Get the subtype.
		Char STORM_FN type() const { return subtype; }

	private:
		// Entity we're referring to.
		Named *entity;

		// Subtype, if any. Char(0) indicates no subtype.
		Char subtype;
	};

}
