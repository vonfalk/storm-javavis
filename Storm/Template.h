#pragma once
#include "Named.h"
#include "Utils/Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Defines something that generated Named objects based on the parameter list given.
	 * An example of use-cases is to implement template types, but it could also be used
	 * to implement template functions and packages among other things.
	 *
	 * If this class is added to a NameSet, it will be queried if no regular Named is found
	 * and therefore has the chance to generate a Named that will be used as a substitute.
	 * Any created Named will be added to the NameSet as usual to ensure proper operation
	 * of the rest of the system.
	 *
	 * TODO: Replace Fn with FnPtr!
	 */
	class Template : public Object {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR Template(Par<Str> name);

		// Ctor.
		Template(const String &name);

		// Ctor.
		Template(const String &name, Fn<Named *, Par<NamePart>> generateFn);

		// Name.
		const String name;

		// Use to generate classes without overriding this class.
		Fn<Named *, Par<NamePart>> generateFn;

		// Called when something with our name is not found. Returns null if nothing is found.
		virtual Named *STORM_FN generate(Par<NamePart> par);
	};

}
