#pragma once
#include "Shared/TObject.h"
#include "Shared/Str.h"
#include "Thread.h"
#include "SrcPos.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Object that all objects used in syntax has to be derived from. Contains
	 * an additional SrcPos that is automatically set by the framework.
	 */
	class SObject : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR SObject();
		STORM_CTOR SObject(SrcPos pos);

		// The position of this SObject. Initialized to nothing.
		STORM_VAR SrcPos pos;
	};


	/**
	 * String object for use in the syntax.
	 */
	class SStr : public SObject {
		STORM_CLASS;
	public:
		STORM_CTOR SStr(Par<Str> src);
		STORM_CTOR SStr(Par<SStr> src);
		STORM_CTOR SStr(Par<Str> src, SrcPos pos);
		SStr(const String &str);
		SStr(const String &str, const SrcPos &pos);

		// The string captured.
		STORM_VAR Auto<Str> v;

		// Allow transforming an SStr.
		Str *STORM_FN transform() const;

	protected:
		virtual void output(wostream &to) const;
	};

	// C++ convenience.
	Auto<SStr> sstr(Engine &e, const String &str);
	Auto<SStr> sstr(Engine &e, const String &str, const SrcPos &pos);
}
