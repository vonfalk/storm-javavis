#include "stdafx.h"
#include "VTablePos.h"

namespace storm {

	wostream &operator <<(wostream &to, const VTablePos &pos) {
		switch (pos.type) {
		case VTablePos::tNone:
			to << L"invalid";
			return to;
		case VTablePos::tStorm:
			to << L"storm";
			break;
		case VTablePos::tCpp:
			to << L"c++";
			break;
		}
		to << L":" << pos.offset;
		return to;
	}


}
