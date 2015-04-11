#include "stdafx.h"
#include "ExceptionWrap.h"
#include "Exception.h"

namespace storm {

	void throwSyntaxError(SrcPos pos, Par<Str> msg) {
		throw SyntaxError(pos, msg->v);
	}

}
