#include "stdafx.h"
#include "Wrap.h"
#include "Exception.h"

namespace storm {

	void throwSyntaxError(SrcPos pos, Str *msg) {
		TODO(L"REMOVE ME!");
		throw new (msg) SyntaxError(pos, msg);
	}

}
