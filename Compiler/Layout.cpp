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

	// Validate the generated GcType.
	static bool validate(const GcType *type) {
		for (nat i = 0; i < type->count; i++) {
			if (type->offset[i] >= type->stride) {
				PLN(L"@" << i << L": " << type->offset[i] << L" >= " << type->stride);
				return false;
			}
		}
		return true;
	}

	nat Layout::fillGcType(Size parentSize, const GcType *parent, GcType *to) {
		Size s = parentSize;
		nat pos = parent ? parent->count : 0;

		// Copy entries from the parent.
		if (to && parent) {
			for (nat i = 0; i < parent->count; i++)
				to->offset[i] = parent->offset[i];
		}

		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);
			Value vType = v->type;
			Size vSize = vType.size();
			s += vSize.align();

			if (vType.isPtr() || vType.ref) {
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

		if (to) {
			assert(validate(to), L"Invalid GcType generated!");
			assert(to->stride == s.current(), L"Size of the provided GcType does not match!");
			assert(pos == to->count, L"Too small GcType provided!");
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
