#include "stdafx.h"
#include "Tree.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		namespace glr {

			TreeNode::TreeNode(Nat pos) : pos(pos), children(null) {}

			TreeNode::TreeNode(Nat pos, Nat production, Nat count) : pos(pos) {
				children = runtime::allocArray<TreeNode *>(engine(), &pointerArrayType, count);
				children->filled = production;
			}

			void TreeNode::toS(StrBuf *to) const {
				*to << L"@" << pos;

				if (children) {
					*to << L" reduced by " << children->filled << L" {\n";
					{
						Indent z(to);
						for (Nat i = 0; i < children->count; i++) {
							children->v[i]->toS(to);
							*to << L"\n";
						}
					}
					*to << L"}";
				}
			}

		}
	}
}
