#include "stdafx.h"
#include "VTablePos.h"

namespace storm {

	wostream &operator <<(wostream &to, const VTablePos &pos) {
		to << (pos.stormEntry ? L"storm" : L"c++");
		to << L":" << pos.offset;
		return to;
	}


}
