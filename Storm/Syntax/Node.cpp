#include "stdafx.h"
#include "Node.h"

namespace storm {
	namespace syntax {

		Node::Node() {}

		Node::Node(Par<Node> o) : pos(o->pos) {}

		Node::Node(SrcPos pos) : pos(pos) {}

	}
}
