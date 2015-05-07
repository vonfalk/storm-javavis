#include "stdafx.h"
#include "FnPtr.h"
#include "TObject.h"
#include "Code/Future.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	FnPtrBase *FnPtrBase::createRaw(Type *type, void *refData,
									Thread *t, Object *thisPtr,
									bool strongThis, bool member) {
		Engine &e = type->engine;
		FnPtrBase *result = new (type) FnPtrBase(code::Ref::fromLea(e.arena, refData), t, member, thisPtr, strongThis);
		type->vtable.update(result);
		return result;
	}

	FnPtrBase::FnPtrBase(const code::Ref &ref, Par<Thread> thread, bool member, Object *thisPtr, bool strongThis) :
		fnRef(ref),
		rawFn(null),
		thisPtr(thisPtr),
		thread(thread),
		strongThisPtr(strongThis),
		isMember(member) {
		init();
	}

	FnPtrBase::FnPtrBase(const void *fn, Object *thisPtr, bool strongThis) :
		fnRef(),
		rawFn(fn),
		thisPtr(thisPtr),
		strongThisPtr(strongThis),
		isMember(thisPtr != null) {
		init();
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

	FnPtrBase::FnPtrBase(Par<FnPtrBase> o) :
		fnRef(o->fnRef),
		rawFn(o->rawFn),
		thread(o->thread),
		thisPtr(o->thisPtr),
		strongThisPtr(o->strongThisPtr),
		isMember(o->isMember) {
		if (strongThisPtr)
			thisPtr->addRef();
	}

	FnPtrBase::~FnPtrBase() {
		if (strongThisPtr)
			thisPtr->release();
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
		return t->thread != code::Thread::current();
	}

	static void doCall(void *output, const BasicTypeInfo &type,
					const code::FnParams &params, Par<Thread> thread,
					const void *toCall, bool member) {

		bool spawn = false;
		if (thread)
			spawn = thread->thread != code::Thread::current();

		if (spawn) {
			code::FutureSema<code::Sema> future(output);
			code::UThread::spawn(toCall, member, params, future, type, &thread->thread);
			future.result();
		} else {
			code::call(toCall, member, params, output, type);
		}
	}

	void FnPtrBase::callRaw(void *out, const BasicTypeInfo &type, const code::FnParams &params, TObject *first) const {
		const void *toCall = rawFn;
		if (toCall == null)
			toCall = fnRef.address();

		Thread *thread = runOn(first);

		if (thisPtr) {
			Auto<Object> tPtr = capture(thisPtr);
			if (needsCopy(first)) {
				Auto<CloneEnv> env = CREATE(CloneEnv, this);
				clone(tPtr, env);
			}

			// TODO: In the case of a thread call, we can avoid this allocation.
			code::FnParams p = params;
			p.addFirst(tPtr.borrow());

			doCall(out, type, p, thread, toCall, isMember);
		} else {
			doCall(out, type, params, thread, toCall, isMember);
		}
	}

}
