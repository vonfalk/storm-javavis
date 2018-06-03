#include "stdafx.h"
#include "DelegatedRef.h"

namespace code {

	DelegatedContent::DelegatedContent(Ref referTo) {
		set(referTo);
	}

	void DelegatedContent::set(Ref referTo) {
		ref = new (this) DelegatedRef(this, referTo);
		set(ref->address(), 0);
	}


	DelegatedRef::DelegatedRef(DelegatedContent *owner, Ref to) :
		Reference(to, owner), owner(owner) {}

	void DelegatedRef::moved(const void *newAddr) {
		owner->set(newAddr, 0);
	}

}
