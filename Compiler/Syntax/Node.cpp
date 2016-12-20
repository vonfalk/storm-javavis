#include "stdafx.h"
#include "Node.h"
#include "Exception.h"
#include "Type.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		Node::Node() {}

		Node::Node(SrcPos pos) : pos(pos) {}

		void Node::throwError() {
			throw SyntaxError(pos, L"Trying to transform a node not representing a match.");
		}

		const void *transformFunction(Type *type, const Value &result, const Value &param) {
			Array<Value> *par = new (type) Array<Value>();
			par->push(thisPtr(type));
			if (param != Value())
				par->push(param);
			SimplePart *part = new (type) SimplePart(new (type) Str(L"transform"), par);
			if (Function *fn = as<Function>(type->find(part))) {
				if (!result.canStore(fn->result)) {
					throw InternalError(L"The function " + ::toS(fn->identifier()) + L" returns " +
										::toS(fn->result) + L", which is incompatible with " +
										::toS(result));
				}

				return fn->ref().address();
			} else {
				throw InternalError(L"Can not find " + ::toS(part) + L" in " + ::toS(type->identifier()));
			}
		}

	}
}
