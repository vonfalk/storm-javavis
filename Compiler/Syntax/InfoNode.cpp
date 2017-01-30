#include "stdafx.h"
#include "InfoNode.h"
#include "Core/Array.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Rule.h"
#include "Production.h"

namespace storm {
	namespace syntax {

		static const Nat invalid = -1;

		InfoNode::InfoNode() {
			color = tNone;
			invalidate();
		}

		Nat InfoNode::length() {
			if (prevLength == invalid)
				prevLength = computeLength();
			return prevLength;
		}

		InfoLeaf *InfoNode::leafAt(Nat pos) {
			return null;
		}

		Nat InfoNode::computeLength() {
			return 0;
		}

		void InfoNode::invalidate() {
			prevLength = invalid;
		}

		Str *InfoNode::format() const {
			StrBuf *to = new (this) StrBuf();
			format(to);
			return to->toS();
		}

		void InfoNode::format(StrBuf *to) const {
			if (color != tNone)
				*to << L" #" << name(engine(), color);
		}

		/**
		 * Internal node.
		 */

		InfoInternal::InfoInternal(Production *p, Nat children) {
			this->prod = p;
			this->children = runtime::allocArray<InfoNode *>(engine(), &pointerArrayType, children);
		}

		InfoLeaf *InfoInternal::leafAt(Nat pos) {
			for (nat i = 0; i < count(); i++) {
				Nat len = at(i)->length();
				if (pos < len)
					return at(i)->leafAt(pos);
				pos -= len;
			}
			return null;
		}

		Nat InfoInternal::computeLength() {
			Nat len = 0;
			for (Nat i = 0; i < count(); i++)
				len += at(i)->length();
			return len;
		}

		void InfoInternal::outOfBounds(Nat v) {
			throw ArrayError(L"Index " + ::toS(v) + L" out of bounds (of " + ::toS(count()) + L").");
		}

		void InfoInternal::toS(StrBuf *to) const {
			for (Nat i = 0; i < children->count; i++)
				children->v[i]->toS(to);
		}

		void InfoInternal::format(StrBuf *to) const {
			*to << L"{";
			{
				Indent z(to);
				if (ProductionType *type = prod->type()) {
					*to << L"\nproduction: " << type->name;
					if (Rule *owner = prod->rule())
						*to << L" of " << owner->name;
				}

				for (Nat i = 0; i < children->count; i++) {
					InfoNode *node = children->v[i];
					*to << L"\n" << node->length() << L" -> ";
					children->v[i]->format(to);
				}
			}
			*to << L"\n}";
			InfoNode::format(to);
		}

		/**
		 * Leaf node.
		 */

		InfoLeaf::InfoLeaf(Str *match) : v(match) {}

		InfoLeaf *InfoLeaf::leafAt(Nat pos) {
			return this;
		}

		Nat InfoLeaf::computeLength() {
			Nat len = 0;
			for (Str::Iter i = v->begin(), e = v->end(); i != e; ++i)
				len++;
			return len;
		}

		void InfoLeaf::toS(StrBuf *to) const {
			*to << v;
		}

		void InfoLeaf::format(StrBuf *to) const {
			*to << v;
			InfoNode::format(to);
		}

	}
}
