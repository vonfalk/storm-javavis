#include "stdafx.h"
#include "Node.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		Node::Node() {}

		Node::Node(Node *o) : pos(o->pos) {}

		Node::Node(SrcPos pos) : pos(pos) {}

		void Node::throwError() {
			throw SyntaxError(pos, L"Trying to transform a node not representing a match.");
		}

	}
}
