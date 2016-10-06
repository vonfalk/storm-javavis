#include "stdafx.h"
#include "Fn.h"
#include "StrBuf.h"
#include "CloneEnv.h"

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


	FnBase::FnBase(const void *fn, RootObject *thisPtr, Bool member, Thread *thread) {
		callMember = member;
		this->thisPtr = thisPtr;
		this->thread = thread;
		new (target()) RawFnTarget(fn);
	}

	FnBase::FnBase(const FnTarget &target, RootObject *thisPtr, Bool member, Thread *thread) {
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
				cloned(o, env);
				thisPtr = o;
			}
		}
	}

	bool FnBase::needsCopy(const TObject *first) const {
		Thread *t = runOn(first);
		if (t)
			return t->thread() != os::Thread::current();
		else
			return false;
	}

	Thread *FnBase::runOn(const TObject *first) const {
		TObject *tObj;
		if (callMember && (tObj = as<TObject>(thisPtr)))
			return tObj->thread;
		else if (callMember && !thread && first)
			return first->thread;
		else
			return thread;
	}

	void FnBase::callRaw(void *out, const BasicTypeInfo &type, os::FnParams &params, const TObject *first, CloneEnv *env) const {
		const void *toCall = target()->ptr();

		Thread *thread = runOn(first);
		bool spawn = needsCopy(first);

		if (thisPtr) {
			RootObject *p = thisPtr;
			if (spawn) {
				if (!env)
					env = new (this) CloneEnv();
				// In this case, 'p' has to be derived from Object.
				p = clone((Object *)p, env);
			}

			params.addFirst(p);
		}

		// Dispatch to the correct thread.

		if (spawn) {
			os::FutureSema<os::Sema> future(out);
			os::UThread::spawn(toCall, callMember, params, future, type, &thread->thread());
			future.result();
		} else {
			os::call(toCall, callMember, params, out, type);
		}
	}

	void FnBase::toS(StrBuf *to) const {
		target()->toS(to);
	}


}
