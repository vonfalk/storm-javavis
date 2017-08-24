#include "stdafx.h"
#include "NewFnCall.h"

namespace os {

	FnCall::FnCall() {}

	FnCall::FnCall(void **params, impl::Thunk thunk, BasicTypeInfo result) {
		this->params(params, false);
		this->thunk = thunk;
		this->result = result;
	}

	FnCall::~FnCall() {
		if (freeParams())
			delete []params();
	}

	void **FnCall::params() const {
		return (void **)(paramsData & ~size_t(0x1));
	}

	bool FnCall::freeParams() const {
		return (paramsData & 0x1) == 0x1;
	}

	void FnCall::params(void **data, bool owner) {
		paramsData = size_t(data);
		assert(!freeParams(), L"'data' is misaligned!");
		if (owner)
			paramsData |= 0x1;
	}

}
