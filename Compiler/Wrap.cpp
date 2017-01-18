#include "stdafx.h"
#include "Wrap.h"
#include "Exception.h"

namespace storm {

	void throwSyntaxError(SrcPos pos, Str *msg) {
		throw SyntaxError(pos, msg->c_str());
	}

}
