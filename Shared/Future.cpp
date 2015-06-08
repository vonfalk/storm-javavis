#include "stdafx.h"
#include "Future.h"
#include "CloneEnv.h"

namespace storm {

	FutureBase::FutureBase(const Handle &type) {
		data = Data::alloc(type);
	}

	FutureBase::FutureBase(const FutureBase *o) {
		data = o->data;
		data->addRef();
	}

	FutureBase::~FutureBase() {
		data->release();
	}

	void FutureBase::deepCopy(Par<CloneEnv> env) {
		// We do not need to do anything here.
	}

	void FutureBase::postRaw(const void *value) {
		// Note: if this is called more than once, we will leak memory here...
		(*data->handle->create)(data->data, value);
		if (data->handle->deepCopy) {
			Auto<CloneEnv> e = CREATE(CloneEnv, this);
			(*data->handle->deepCopy)(data->data, e.borrow());
		}
		data->future.posted();
	}

	void FutureBase::error() {
		data->future.error();
	}

	void FutureBase::resultRaw(void *to) {
		data->future.result();
		(*data->handle->create)(to, data->data);
		if (data->handle->deepCopy) {
			Auto<CloneEnv> e = CREATE(CloneEnv, this);
			(*data->handle->deepCopy)(to, e.borrow());
		}
	}

	os::FutureBase *FutureBase::rawFuture() {
		if (atomicCAS(data->releaseOnResult, 0, 1) == 0)
			data->addRef();
		return &data->future;
	}

	FutureBase::FutureSema::FutureSema(void *data) : os::FutureSema<Sema>(data) {}

	void FutureBase::FutureSema::notify() {
		os::FutureSema<Sema>::notify();
		Data::resultPosted(this);
	}

	FutureBase::Data *FutureBase::Data::alloc(const Handle &type) {
		Data *r = (Data *)malloc(sizeof(Data) + type.size - sizeof(byte));
		r->refs = 1;
		r->handle = &type;
		new (&r->future)FutureSema(r->data);
		r->releaseOnResult = 0;
		return r;
	}

	void FutureBase::Data::free() {
		// If the result is initialized, destroy it as well!
		if (future.dataPosted() && handle->destroy)
			(*handle->destroy)(data);
		future.~FutureSema();
		::free(this);
	}

	void FutureBase::Data::addRef() {
		atomicIncrement(refs);
	}

	void FutureBase::Data::release() {
		if (atomicDecrement(refs) == 0)
			free();
	}

	void FutureBase::Data::resultPosted(FutureSema *sema) {
		Data *me = BASE_PTR(Data, sema, future);

		if (atomicCAS(me->releaseOnResult, 1, 0) == 1)
			me->release();
	}

}
