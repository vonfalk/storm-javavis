#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/SrcPos.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Base class for all syntax nodes. This class will be overridden by 'Rule' to add the
		 * 'transform' function and then by 'Option' to implement the 'transform' function for that
		 * specific option.
		 *
		 * TODO: Always run on compiler thread?
		 */
		class Node : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Node();
			STORM_CTOR Node(Node *o);
			STORM_CTOR Node(SrcPos pos);

			SrcPos pos;

			// Throw an exception. Used when calling 'transform' from a pure Rule object.
			void CODECALL throwError();
		};

	}
}
