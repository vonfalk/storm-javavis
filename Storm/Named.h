#pragma once

namespace storm {

	/**
	 * Denotes a named object in the compiler. Named objects
	 * are anything in the compiler with a name, eg functions,
	 * types among others.
	 */
	class Named : public Printable, NoCopy {
	public:
		inline Named(const String &name) : name(name) {}

		// Our name.
		const String name;
	};

}
