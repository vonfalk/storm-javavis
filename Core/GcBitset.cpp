#include "stdafx.h"
#include "GcBitset.h"
#include "Runtime.h"

namespace storm {

	const GcType bitsetType = {
		GcType::tArray,
		null,
		null,
		sizeof(byte),
		0,
		{},
	};

	GcBitset *allocBitset(Engine &e, nat count) {
		GcBitset *r = (GcBitset *)runtime::allocArray<byte>(e, &bitsetType, count * CHAR_BIT);
		r->filled = count;
		return r;
	}

}
