#include "stdafx.h"
#include "Layout.h"
#include "Core/GcType.h"
#include "Type.h"

namespace storm {

	Layout::Layout() {
		vars = new (this) Array<MemberVar *>();
	}

	void Layout::add(Named *n) {
		if (MemberVar *v = as<MemberVar>(n))
			add(v);
	}

	void Layout::add(MemberVar *v) {
		vars->push(v);
		// The layout is lazily created. 'v' will ask us when we need to lay it out.
	}

	Size Layout::doLayout(Size parentSize) {
		Size s = parentSize;

		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);

			Size vSize = v->type.size();
			s += vSize.align();
			v->setOffset(Offset(s));

			s += vSize;
		}

		return s;
	}

	nat Layout::fillGcType(const GcType *parent, GcType *to) {
		// Note: since we're initializing the size with only the current size, we can not use these
		// computations to actually save the layout while we're producing the GcType.
		Size s(parent->stride);
		nat pos = parent->count;

		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);
			Value vType = v->type;
			Size vSize = vType.size();
			s += vSize.align();

			if (vType.isClass() || vType.isActor() || vType.ref) {
				// We need to GC this one!
				if (to)
					to->offset[pos] = s.current();
				pos++;
			} else if (vType.isValue()) {
				// Copy and offset all members of this value.
				const GcType *src = vType.type->gcType();
				for (nat j = 0; j < src->count; j++) {
					if (to)
						to->offset[pos] = s.current() + src->offset[j];
					pos++;
				}
			}

			s += vSize;
		}

		return pos;
	}

	Array<MemberVar *> *Layout::variables() {
		Array<MemberVar *> *r = new (this) Array<MemberVar *>();
		for (nat i = 0; i < vars->count(); i++) {
			r->push(vars->at(i));
		}
		return r;
	}

}
