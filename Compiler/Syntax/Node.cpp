#include "stdafx.h"
#include "Node.h"
#include "Exception.h"
#include "Type.h"
#include "Engine.h"
#include "Package.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		Node::Node() {}

		Node::Node(SrcPos pos) : pos(pos) {}

		void Node::throwError() {
			throw SyntaxError(pos, L"Trying to transform a node not representing a match.");
		}

		Array<Node *> *Node::children() {
			return new (this) Array<Node *>();
		}

		Array<Node *> *Node::allChildren() {
			Array<Node *> *result = new (this) Array<Node *>();
			allChildren(result, null);
			return result;
		}

		Array<Node *> *Node::allChildren(Type *type) {
			Array<Node *> *result = new (this) Array<Node *>();
			allChildren(result, type);
			return result;
		}

		void Node::allChildren(Array<Node *> *result, MAYBE(Type *) type) {
			Array<Node *> *here = children();
			for (Nat i = 0; i < here->count(); i++) {
				Node *at = here->at(i);

				if (!type || runtime::typeOf(at)->isA(type))
					result->push(at);

				at->allChildren(result, type);
			}
		}


		const void *transformFunction(Type *type, const Value &result, const Value &param1, const Value &param2) {
			Scope root = type->engine.scope();
			Array<Value> *par = new (type) Array<Value>();
			par->push(thisPtr(type));
			if (param1 != Value())
				par->push(param1);
			if (param2 != Value())
				par->push(param2);
			SimplePart *part = new (type) SimplePart(new (type) Str(L"transform"), par);
			if (Function *fn = as<Function>(type->find(part, root))) {
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
