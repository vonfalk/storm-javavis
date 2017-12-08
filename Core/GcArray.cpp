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

	const GcType sizeArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(const void *),
		0,
		{},
	};

	const GcType wordArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(Word),
		0,
		{},
	};

	const GcType natArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(Nat),
		0,
		{},
	};

	const GcType wcharArrayType = {
		GcType::tArray,
		null,
		null,
		sizeof(wchar),
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
