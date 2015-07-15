#include "stdafx.h"
#include "FnPtr.h"
#include "Shared/TObject.h"
#include "OS/Future.h"

namespace storm {

	FnPtrBase::Target::~Target() {}

	RawTarget::RawTarget(const void *fn) : fn(fn) {}

	void RawTarget::cloneTo(void *to, size_t size) const {
		assert(size >= sizeof(RawTarget));
		new (to) RawTarget(*this);
	}

	const void *RawTarget::target() const {
		return fn;
	}

	void RawTarget::output(wostream &to) const {
		to << L"<unknown @" << fn << L">";
	}

	FnPtrBase::FnPtrBase(const Target &target, Par<Thread> thread, bool member, Object *thisPtr, bool strongThis) :
		thisPtr(thisPtr),
		thread(thread),
		strongThisPtr(strongThis),
		isMember(member) {

		target.cloneTo(targetData, targetSize);
		init();
	}

	FnPtrBase::FnPtrBase(const void *fn, Object *thisPtr, bool strongThis) :
		thisPtr(thisPtr),
		thread(thread),
		strongThisPtr(strongThis),
		isMember(thisPtr != null) {

		RawTarget(fn).cloneTo(targetData, targetSize);
		init();
	}

	FnPtrBase::FnPtrBase(Par<FnPtrBase> o) :
		thread(o->thread),
		thisPtr(o->thisPtr),
		strongThisPtr(o->strongThisPtr),
		isMember(o->isMember) {

		o->target()->cloneTo(targetData, targetSize);
		if (strongThisPtr)
			thisPtr->addRef();
	}

	FnPtrBase::~FnPtrBase() {
		if (strongThisPtr)
			thisPtr->release();

		target()->~Target();
	}

	const FnPtrBase::Target *FnPtrBase::target() const {
		const void *d = targetData;
		return (const Target *)d;
	}

	void FnPtrBase::output(wostream &to) const {
		to << L"fn(";
		vector<ValueData> params = typeParams(myType);
		for (nat i = 1; i < params.size(); i++) {
			if (i != 1)
				to << ", ";
			to << params[i];
		}
		to << L")->";
		if (params.size() > 0)
			to << params[0];
		else
			to << ValueData();

		to << L": ";
		target()->output(to);
	}

	void FnPtrBase::init() {
		if (strongThisPtr)
			thisPtr->addRef();

		// Note: do not use dynamic_cast here, as the this-ptr is sometimes not fully initialized
		// when we are called... Storm's type system is, however, initialized earlier and therefore
		// we can safely use as<>.
		if (thread == null)
			if (TObject *t = as<TObject>(thisPtr))
				thread = t->thread;
	}

	void FnPtrBase::deepCopy(Par<CloneEnv> env) {
		clone(thread, env);

		if (thisPtr) {
			Auto<Object> v = capture(thisPtr);
			clone(v, env);

			if (strongThisPtr) {
				thisPtr->release();
				thisPtr = v.ret();
			} else {
				TODO(L"Is this always OK?");
				thisPtr = v.borrow();
			}
		}
	}

	Thread *FnPtrBase::runOn(TObject *first) const {
		Thread *t = thread.borrow();
		if (isMember && !t && first)
			t = first->thread.borrow();
		return t;
	}

	bool FnPtrBase::needsCopy(TObject *first) const {
		Thread *t = runOn(first);
		if (!t)
			return false;
		return t->thread != os::Thread::current();
	}

	static void doCall(void *output, const BasicTypeInfo &type,
					const os::FnParams &params, Par<Thread> thread,
					const void *toCall, bool member) {

		bool spawn = false;
		if (thread)
			spawn = thread->thread != os::Thread::current();

		if (spawn) {
			os::FutureSema<os::Sema> future(output);
			os::UThread::spawn(toCall, member, params, future, type, &thread->thread);
			future.result();
		} else {
			os::call(toCall, member, params, output, type);
		}
	}

	void FnPtrBase::callRaw(void *out, const BasicTypeInfo &type, const os::FnParams &params, TObject *first) const {
		const void *toCall = target()->target();

		Thread *thread = runOn(first);

		if (thisPtr) {
			Auto<Object> tPtr = capture(thisPtr);
			if (needsCopy(first)) {
				// Note: in case 'thisPtr' is a threaded object, cloning is a no-op!
				Auto<CloneEnv> env = CREATE(CloneEnv, this);
				clone(tPtr, env);
			}

			// TODO: In the case of a thread call, we can avoid this allocation.
			os::FnParams p = params;
			p.addFirst(tPtr.borrow());

			doCall(out, type, p, thread, toCall, isMember);
		} else {
			doCall(out, type, params, thread, toCall, isMember);
		}
	}

}
