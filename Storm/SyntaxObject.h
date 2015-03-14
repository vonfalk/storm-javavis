#pragma once
#include "Lib/Object.h"
#include "SrcPos.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Object that all objects used in syntax has to be derived from. Contains
	 * an additional SrcPos that is automatically set by the framework.
	 */
	class SObject : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR SObject();

		// The position of this SObject. Initialized to nothing.
		SrcPos pos;
	};


	/**
	 * String object for use in the syntax.
	 */
	class SStr : public SObject {
		STORM_CLASS;
	public:
		STORM_CTOR SStr(Par<Str> src);
		STORM_CTOR SStr(Par<SStr> src);
		SStr(const String &str);

		// The string captured.
		Auto<Str> v;

		// Get the string (TODO: Expose 'v' instead).
		Str *STORM_FN str();

		// Equals.
		virtual Bool STORM_FN equals(Object *o);

	protected:
		virtual void output(wostream &to) const;
	};

}
