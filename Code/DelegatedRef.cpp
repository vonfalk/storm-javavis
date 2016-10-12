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


	DelegatedSrc::DelegatedSrc(Ref referTo, Str *name) : RefSource(name) {
		set(referTo);
	}

	void DelegatedSrc::set(Ref referTo) {
		RefSource::set(new (this) DelegatedContent(referTo));
	}


	DelegatedRef::DelegatedRef(DelegatedContent *owner, Ref to) :
		Reference(to, owner), owner(owner) {}

	void DelegatedRef::moved(const void *newAddr) {
		owner->set(newAddr, 0);
	}

}
