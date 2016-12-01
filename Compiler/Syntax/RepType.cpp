#include "stdafx.h"
#include "RepType.h"

namespace storm {
	namespace syntax {

		StrBuf &operator <<(StrBuf &to, RepType r) {
			switch (r) {
			case repNone:
				to << L"repNone";
				break;
			case repZeroOne:
				to << L"repZeroOne";
				break;
			case repOnePlus:
				to << L"repOnePlus";
				break;
			case repZeroPlus:
				to << L"repZeroPlus";
				break;
			}
			return to;
		}

		Bool skippable(RepType r) {
			switch (r) {
			case repZeroOne:
			case repZeroPlus:
				return true;
			default:
				return false;
			}
		}

		Bool repeatable(RepType r) {
			switch (r) {
			case repOnePlus:
			case repZeroPlus:
				return true;
			default:
				return false;
			}
		}
	}
}
