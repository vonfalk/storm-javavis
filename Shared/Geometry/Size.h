#pragma once

namespace storm {
	namespace geometry {
		STORM_PKG(core.geometry);

		/**
		 * Size in 2D-space.
		 */
		class Size {
			STORM_VALUE;
		public:
			STORM_CTOR Size();
			STORM_CTOR Size(Nat w, Nat h);

			STORM_VAR Nat w;
			STORM_VAR Nat h;
		};

	}
}
