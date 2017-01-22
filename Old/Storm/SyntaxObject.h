#pragma once
#include "Shared/Str.h"
#include "Shared/CloneEnv.h"
#include "SrcPos.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * String object for use in the syntax.
	 */
	class SStr : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR SStr(Par<Str> src);
		STORM_CTOR SStr(Par<SStr> src);
		STORM_CTOR SStr(Par<Str> src, SrcPos pos);
		SStr(const String &str);
		SStr(const String &str, const SrcPos &pos);

		// Position of this string.
		STORM_VAR SrcPos pos;

		// The string captured.
		STORM_VAR Auto<Str> v;

		// Allow transforming an SStr.
		Str *STORM_FN transform() const;

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	protected:
		virtual void output(wostream &to) const;
	};

	// C++ convenience.
	Auto<SStr> sstr(Engine &e, const String &str);
	Auto<SStr> sstr(Engine &e, const String &str, const SrcPos &pos);
}
