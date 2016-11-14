#include "stdafx.h"
#include "VTableSlot.h"
#include "VTableCpp.h"
#include "Core/StrBuf.h"

namespace storm {

	VTableSlot::VTableSlot() : type(tNone), offset(0) {}

	VTableSlot::VTableSlot(VType t, Nat offset) : type(t), offset(offset) {
		if (offset == vtable::invalid)
			type = tNone;
	}

	VTableSlot cppSlot(Nat offset) {
		return VTableSlot(VTableSlot::tCpp, offset);
	}

	VTableSlot stormSlot(Nat offset) {
		return VTableSlot(VTableSlot::tStorm, offset);
	}

	wostream &operator <<(wostream &to, const VTableSlot &pos) {
		switch (pos.type) {
		case VTableSlot::tNone:
			to << L"invalid";
			return to;
		case VTableSlot::tStorm:
			to << L"storm";
			break;
		case VTableSlot::tCpp:
			to << L"c++";
			break;
		}
		to << L":" << pos.offset;
		return to;
	}

	StrBuf &operator <<(StrBuf &to, VTableSlot pos) {
		switch (pos.type) {
		case VTableSlot::tNone:
			to << L"invalid";
			return to;
		case VTableSlot::tStorm:
			to << L"storm";
			break;
		case VTableSlot::tCpp:
			to << L"c++";
			break;
		}
		to << L":" << pos.offset;
		return to;
	}

}
