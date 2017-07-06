#include "stdafx.h"
#include "FnCall.h"

namespace os {

	FnCallRaw::FnCallRaw() {}

	FnCallRaw::FnCallRaw(void **params, CallThunk thunk) {
		this->params(params, false);
		this->thunk = thunk;
	}

#ifdef USE_MOVE
	FnCallRaw::FnCallRaw(FnCallRaw &&o) : paramsData(o.paramsData), thunk(o.thunk) {
		o.paramsData = 0;
	}

	FnCallRaw &FnCallRaw::operator =(FnCallRaw &&o) {
		if (freeParams())
			delete []params();
		this->paramsData = o.paramsData;
		this->thunk = o.thunk;
		o.paramsData = 0;
		return *this;
	}
#endif

	FnCallRaw::~FnCallRaw() {
		if (freeParams())
			delete []params();
	}

	void **FnCallRaw::params() const {
		return (void **)(paramsData & ~size_t(0x1));
	}

	bool FnCallRaw::freeParams() const {
		return (paramsData & 0x1) == 0x1;
	}

	void FnCallRaw::params(void **data, bool owner) {
		paramsData = size_t(data);
		assert(!freeParams(), L"'data' is misaligned!");
		if (owner)
			paramsData |= 0x1;
	}

}
