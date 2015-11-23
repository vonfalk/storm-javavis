#pragma once
#include "Code/Reference.h"
#include "Shared/TObject.h"
#include "Shared/EnginePtr.h"
#include "Thread.h"

namespace storm {

	namespace wrap {

		// Wrapper for the reference system.
		STORM_PKG(core.asm);

		/**
		 * RefSource
		 * TODO: Implement!
		 */


		/**
		 * Reference.
		 * TODO: Implement!
		 */

		/**
		 * Ref.
		 */
		class Ref {
			STORM_VALUE;
		public:
			Ref(const code::Ref &ref);

			code::Ref v;
		};

		wostream &operator <<(wostream &to, const Ref &r);
		Str *STORM_ENGINE_FN toS(EnginePtr e, Ref r);

	}

}
