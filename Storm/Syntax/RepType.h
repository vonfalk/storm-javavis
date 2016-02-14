#pragma once
#include "Shared/CloneEnv.h"
#include "Shared/EnginePtr.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.syntax);

		/**
		 * Kind of repeat for an option. This should be a simple enum whenever Storm properly
		 * supports that.
		 */
		class RepType {
			STORM_VALUE;
		public:
			// Underlying enum.
			enum V {
				repNone,
				repZeroOne,
				repOnePlus,
				repZeroPlus,
			};

			// Default ctor.
			STORM_CTOR RepType();

			// Create with a given value. Prefer factory functions below.
			RepType(V v);

			// Value.
			V v;

			// Compare.
			inline Bool STORM_FN operator ==(RepType o) const {
				return v == o.v;
			}

			inline Bool STORM_FN operator !=(RepType o) const {
				return v != o.v;
			}

			// Deep copy.
			void STORM_FN deepCopy(Par<CloneEnv> env);
		};

		// Output.
		wostream &operator <<(wostream &to, const RepType &m);
		Str *STORM_ENGINE_FN toS(EnginePtr e, RepType m);

		// Create repeat types.
		RepType repNone();
		RepType repZeroOne();
		RepType repOnePlus();
		RepType repZeroPlus();

	}
}
