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

	Ref DelegatedContent::to() const {
		return Ref(ref);
	}

	StolenContent::StolenContent(RefSource *by) : DelegatedContent(Ref(by)) {}

	RefSource *StolenContent::stolenBy() const {
		return ref->source();
	}


	DelegatedRef::DelegatedRef(DelegatedContent *owner, Ref to) :
		Reference(to, owner), owner(owner) {}

	void DelegatedRef::moved(const void *newAddr) {
		owner->set(newAddr, 0);
	}

}
