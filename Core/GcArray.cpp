#include "stdafx.h"
#include "GcArray.h"

namespace storm {

	const GcType pointerArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(const void *),
		1,
		{ 0 },
	};

}
