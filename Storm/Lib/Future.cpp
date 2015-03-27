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

	FutureBase::Data *FutureBase::Data::alloc(const Handle &type) {
		Data *r = (Data *)malloc(sizeof(Data) + type.size - sizeof(byte));
		r->refs = 1;
		r->handle = &type;
		new (&r->future)code::FutureSema<Sema>(r->data);
		new (&r->sync)Sema(1);
		return r;
	}

	void FutureBase::Data::free() {
		// If the result is initialized, destroy it as well!
		if (future.dataPosted() && handle->destroy)
			(*handle->destroy)(data);
		future.~FutureSema<Sema>();
		sync.~Sema();
		::free(this);
	}

	void FutureBase::Data::addRef() {
		atomicIncrement(refs);
	}

	void FutureBase::Data::release() {
		if (atomicDecrement(refs) == 0)
			free();
	}

}
