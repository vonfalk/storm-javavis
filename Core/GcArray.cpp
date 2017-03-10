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

	const GcType natArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(Nat),
		0,
		{},
	};

	const GcType byteArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(Byte),
		0,
		{},
	};
}
