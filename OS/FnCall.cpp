#include "stdafx.h"
#include "FnCall.h"

namespace os {

	FnCallRaw::FnCallRaw() {}

	FnCallRaw::FnCallRaw(void **params, CallThunk thunk) {
		this->params(params, false);
		this->thunk = thunk;
	}

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
