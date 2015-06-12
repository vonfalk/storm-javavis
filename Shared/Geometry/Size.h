#pragma once
#include "EnginePtr.h"

namespace storm {
	class Str;

	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * Size in 2D-space. If either 'w' or 'h' is negative, the size is considered invalid.
		 */
		class Size {
			STORM_VALUE;
		public:
			STORM_CTOR Size();
			STORM_CTOR Size(Int wh);
			STORM_CTOR Size(Int w, Int h);

			STORM_VAR Int w;
			STORM_VAR Int h;

			Bool STORM_FN valid() const;
		};

		Size STORM_FN operator +(Size a, Size b);
		Size STORM_FN operator -(Size a, Size b);
		Size STORM_FN operator *(Int s, Size a);
		Size STORM_FN operator *(Size a, Int s);

		inline Bool STORM_FN operator ==(Size a, Size b) { return a.w == b.w && a.h == b.h; }
		inline Bool STORM_FN operator !=(Size a, Size b) { return !(a == b); }

		wostream &operator <<(wostream &to, const Size &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Size s);
	}
}
