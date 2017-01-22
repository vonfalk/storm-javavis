#pragma once
#include "Shared/TObject.h"
#include "Storm/Thread.h"
#include "Storm/SrcPos.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Base class for all syntax nodes. This class will be overridden by 'Rule' to add the
		 * 'transform' function, and then by 'Option' that implements the 'transform' function for
		 * that specific option.
		 */
		class Node : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Node();
			STORM_CTOR Node(Par<Node> o);
			STORM_CTOR Node(SrcPos pos);

			STORM_VAR SrcPos pos;
		};

	}
}
