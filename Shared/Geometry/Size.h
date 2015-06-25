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
			STORM_CAST_CTOR Size(Float wh);
			STORM_CTOR Size(Float w, Float h);

			STORM_VAR Float w;
			STORM_VAR Float h;

			Bool STORM_FN valid() const;
		};

		Size STORM_FN operator +(Size a, Size b);
		Size STORM_FN operator -(Size a, Size b);
		Size STORM_FN operator *(Float s, Size a);
		Size STORM_FN operator *(Size a, Float s);
		Size STORM_FN operator /(Size a, Float s);

		inline Bool STORM_FN operator ==(Size a, Size b) { return a.w == b.w && a.h == b.h; }
		inline Bool STORM_FN operator !=(Size a, Size b) { return !(a == b); }

		Size STORM_FN abs(Size a);

		wostream &operator <<(wostream &to, const Size &s);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Size s);
	}
}
