#include "stdafx.h"
#include "Fn.h"
#include "StrBuf.h"

namespace storm {

	RawFnTarget::RawFnTarget(const void *ptr) : data(ptr) {}

	void RawFnTarget::cloneTo(void *to, size_t size) const {
		assert(size >= sizeof(*this));
		new (to) RawFnTarget(data);
	}

	const void *RawFnTarget::ptr() const {
		return data;
	}

	void RawFnTarget::toS(StrBuf *to) const {
		*to << L"C++ function @" << data;
	}


	FnBase::FnBase(const void *fn, RootObject *thisPtr) {
		thread = null;
		this->thisPtr = thisPtr;
		callMember = thisPtr != null;
		new (target()) RawFnTarget(fn);
	}

	FnBase::FnBase(const Target &target, RootObject *thisPtr, Bool member, Thread *thread) {
		callMember = member;
		this->thisPtr = thisPtr;
		this->thread = thread;
		target.cloneTo(this->target(), targetSize);
	}

	FnBase::FnBase(FnBase *o) {
		callMember = o->callMember;
		thisPtr = o->thisPtr;
		thread = o->thread;
		o->target()->cloneTo(target(), targetSize);
	}

	void FnBase::deepCopy(CloneEnv *env) {
		if (thisPtr) {
			if (Object *o = as<Object>(thisPtr)) {
				// Yes, we need to clone it!
				cloned(env, o);
				thisPtr = o;
			}
		}
	}

	void FnBase::toS(StrBuf *to) const {
		target()->toS(to);
	}


}
