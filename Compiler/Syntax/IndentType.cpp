#include "stdafx.h"
#include "IndentType.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		StrBuf &STORM_FN operator <<(StrBuf &to, IndentType i) {
			switch (i) {
			case indentNone:
				to << L"none";
				break;
			case indentIncrease:
				to << L"+";
				break;
			case indentDecrease:
				to << L"-";
				break;
			case indentAlign:
				to << L"@";
				break;
			}
			return to;
		}

	}
}
