#include "stdafx.h"
#include "Layout.h"

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

}
