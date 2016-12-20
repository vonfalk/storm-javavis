#pragma once
#include "Core/Object.h"
#include "Core/Str.h"
#include "Compiler/SrcPos.h"

namespace storm {
	namespace syntax {
		STORM_PKG(core.lang);

		/**
		 * String object for use in the syntax.
		 */
		class SStr : public Object {
			STORM_CLASS;
		public:
			SStr(const wchar *src);
			SStr(const wchar *src, SrcPos pos);

			STORM_CTOR SStr(Str *src);
			STORM_CTOR SStr(Str *src, SrcPos pos);

			// Position of this string.
			SrcPos pos;

			// String.
			Str *v;

			// Allow transforming an SStr.
			Str *STORM_FN transform() const;

			// Deep copy.
			virtual void STORM_FN deepCopy(CloneEnv *env);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
