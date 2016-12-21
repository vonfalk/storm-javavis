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
			STORM_CTOR Node(SrcPos pos);

			SrcPos pos;

			// Throw an exception. Used when calling 'transform' from a pure Rule object.
			void CODECALL throwError();
		};


		/**
		 * Transform syntax nodes in C++. Assumes the result is a pointer.
		 */

		// Find the appropriate 'transform' function in a specific type. If 'param' is void, no
		// params are assumed except the this pointer.
		const void *transformFunction(Type *type, const Value &result, const Value &param);

		template <class R>
		R *transformNode(Node *node) {
			Engine &e = node->engine();
			const void *fn = transformFunction(runtime::typeOf(node), Value(StormInfo<R>::type(e)), Value());
			os::FnParams p;
			p.add(node);
			return os::call<R *>(fn, true, p);
		}


		template <class R, class P>
		R *transformNode(Node *node, P par) {
			Engine &e = node->engine();
			const void *fn = transformFunction(runtime::typeOf(node),
											Value(StormInfo<R>::type(e)),
											Value(StormInfo<P>::type(e)));
			os::FnParams p;
			p.add(node);
			p.add(par);
			return os::call<R *>(fn, true, p);
		}

	}
}