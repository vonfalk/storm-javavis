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
		nat bytes = (count + CHAR_BIT - 1) / CHAR_BIT;
		GcBitset *r = (GcBitset *)runtime::allocArray<byte>(e, &bitsetType, bytes);
		r->filled = count;
		return r;
	}

}
