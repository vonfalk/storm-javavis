#include "stdafx.h"
#include "GcType.h"
#include "Runtime.h"
#include "Str.h"

namespace storm {

	void printGcType(const GcType *type) {
		PLN(L"Gc type for " << runtime::typeName(type->type));
		switch (type->kind) {
		case GcType::tFixed:
			PLN(L"  kind: fixed");
			break;
		case GcType::tFixedObj:
			PLN(L"  kind: fixed object");
			break;
		case GcType::tType:
			PLN(L"  kind: type");
			break;
		case GcType::tArray:
			PLN(L"  kind: array");
			break;
		case GcType::tWeakArray:
			PLN(L"  kind: weak array");
			break;
		default:
			PLN(L"  kind: unknown (" << type->kind << L")");
			break;
		}
		PLN(L"  stride: " << type->stride);
		PLN(L"  finalizer: " << type->finalizer);
		PNN(L"  offsets: ");
		for (Nat i = 0; i < type->count; i++) {
			if (i > 0)
				PNN(L", ");
			PNN(type->offset[i]);
		}
		PLN(L"");
	}

}
