#include "stdafx.h"
#include "Future.h"
#include "CloneEnv.h"
#include "Utils/Memory.h"

namespace storm {

	FutureBase::FutureBase(const Handle &type) : handle(type) {
		data = new Data();
		result = (GcArray<byte> *)runtime::allocArray(engine(), type.gcArrayType, 1);
	}

	FutureBase::FutureBase(const FutureBase &o) : data(o.data), handle(o.handle), result(o.result) {
		data->addRef();
	}

	FutureBase::~FutureBase() {
		if (data->release()) {
			// We are the last one, see if we should free our result as well.
			// TODO: This can be solved nicer when we can attach destructors to arrays.
			if (result->filled) {
				handle.safeDestroy(result->v);
			}
		}
	}

	void FutureBase::deepCopy(CloneEnv *env) {
		// We do not need to do anything here.
	}

	void FutureBase::postRaw(const void *value) {
		// Do not post multiple times.
		if (atomicCAS(result->filled, 0, 1) == 0) {
			handle.safeCopy(result->v, value);
			if (handle.deepCopyFn) {
				CloneEnv *e = new (this) CloneEnv();
				(*handle.deepCopyFn)(result->v, e);
			}
			data->future.posted();
		} else {
			WARNING(L"Trying to re-use a future!");
		}
	}

	void FutureBase::error() {
		if (atomicCAS(result->filled, 0, 1) == 0) {
			data->future.error();
		} else {
			WARNING(L"Trying to re-use a future!");
		}
	}

	void FutureBase::resultRaw(void *to) {
		data->future.result();
		handle.safeCopy(to, result->v);
		if (handle.deepCopyFn) {
			CloneEnv *e = new (this) CloneEnv();
			(*handle.deepCopyFn)(to, e);
		}
	}

	os::FutureBase *FutureBase::rawFuture() {
		if (atomicCAS(data->releaseOnResult, 0, 1) == 0)
			data->addRef();
		return &data->future;
	}

	FutureBase::FutureSema::FutureSema() : os::FutureSema<os::Sema>() {}

	void FutureBase::FutureSema::notify() {
		os::FutureSema<os::Sema>::notify();
		Data::resultPosted(this);
	}

	FutureBase::Data::Data() : refs(1), releaseOnResult(0) {}

	FutureBase::Data::~Data() {}

	void FutureBase::Data::addRef() {
		atomicIncrement(refs);
	}

	bool FutureBase::Data::release() {
		if (atomicDecrement(refs) == 0) {
			delete this;
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
