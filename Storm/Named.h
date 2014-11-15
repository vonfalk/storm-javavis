#pragma once
#include "Lib/Object.h"
#include "Name.h"

namespace storm {
	STORM_PKG(core.lang);

	class Named;

	/**
	 * An interface for objects that can lookup names.
	 */
	class NameLookup : public Object {
		STORM_CLASS;
	public:
		// Find the specified name in here, returns null if not found.
		virtual Named *find(const Name &name);

		// Get the parent object to this lookup, or null if none.
		virtual NameLookup *parent() const;
	};


	/**
	 * Denotes a named object in the compiler. Named objects
	 * are anything in the compiler with a name, eg functions,
	 * types among others.
	 */
	class Named : public NameLookup {
		STORM_CLASS;
	public:
		STORM_CTOR Named(Auto<Str> name);
		inline Named(const String &name) : name(name) {}

		// Our name.
		const String name;
	};

}
