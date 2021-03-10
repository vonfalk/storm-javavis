#include "stdafx.h"
#include "Future.h"
#include "CloneEnv.h"
#include "Utils/Memory.h"

namespace storm {

	FutureBase::FutureBase(const Handle &type) : dataClone(0), handle(type), result(null) {
		data(new (runtime::allocStaticRaw(engine(), &Data::gcType)) Data());
		result = (GcArray<byte> *)runtime::allocArray(engine(), type.gcArrayType, 1);
	}

	FutureBase::FutureBase(const FutureBase &o) : dataClone(o.dataClone), handle(o.handle), result(o.result) {
		data()->addRef();
	}

	FutureBase::~FutureBase() {
		if (data()->release()) {
			// We are the last one, see if we should free our result as well.
			// TODO: This can be solved nicer when we can attach destructors to arrays.
			if (result->filled) {
				handle.safeDestroy(result->v);
			}
		}
	}

	void FutureBase::deepCopy(CloneEnv *env) {
		// We need to clone the result now that we have been cloned.
		setClone();
	}

	void FutureBase::setNoClone() {
		dataClone |= size_t(0x1);
	}

	void FutureBase::postRaw(const void *value) {
		// Do not post multiple times.
		if (atomicCAS(result->filled, 0, 1) == 0) {
			handle.safeCopy(result->v, value);
			if (!noClone() && handle.deepCopyFn) {
				CloneEnv *e = new (this) CloneEnv();
				(*handle.deepCopyFn)(result->v, e);
			}
			data()->future.posted();
		} else {
			WARNING(L"Trying to re-use a future!");
		}
	}

	void FutureBase::error() {
		if (atomicCAS(result->filled, 0, 1) == 0) {
			data()->future.error();
		} else {
			WARNING(L"Trying to re-use a future!");
		}
	}

	static os::PtrThrowable *cloneEx(os::PtrThrowable *src, void *) {
		if (Object *o = dynamic_cast<Object *>(src)) {
			return clone(o);
		} else {
			return src;
		}
	}

	void FutureBase::resultRaw(void *to) {
		data()->future.result(&cloneEx, null);

		handle.safeCopy(to, result->v);
		if (!noClone() && handle.deepCopyFn) {
			CloneEnv *e = new (this) CloneEnv();
			(*handle.deepCopyFn)(to, e);
		}
	}

	void FutureBase::errorResult() {
		data()->future.result(&cloneEx, null);
	}

	void FutureBase::detach() {
		data()->future.detach();
	}

	os::FutureBase *FutureBase::rawFuture() {
		if (atomicCAS(data()->releaseOnResult, 0, 1) == 0)
			data()->addRef();
		return &data()->future;
	}

	FutureBase::FutureSema::FutureSema() : os::FutureSema<os::Sema>() {}

	void FutureBase::FutureSema::notify() {
		os::FutureSema<os::Sema>::notify();
		Data::resultPosted(this);
	}

	const GcType FutureBase::Data::gcType = {
		GcType::tFixed,
		null,
		null,
		sizeof(FutureBase::Data),
		1,
		{ OFFSET_OF(FutureBase::Data, future.ptrException) }
	};

	FutureBase::Data::Data() : refs(1), releaseOnResult(0) {}

	FutureBase::Data::~Data() {}

	void FutureBase::Data::addRef() {
		atomicIncrement(refs);
	}

	bool FutureBase::Data::release() {
		if (atomicDecrement(refs) == 0) {
			this->~Data();
			return true;
		} else {
			return false;
		}
	}

	void FutureBase::Data::resultPosted(FutureSema *sema) {
		Data *me = BASE_PTR(Data, sema, future);

		if (atomicCAS(me->releaseOnResult, 1, 0) == 1)
			me->release();
	}


}
