#include "stdafx.h"
#include "RepType.h"
#include "Shared/Str.h"

namespace storm {
	namespace syntax {

		RepType::RepType() : v(repNone) {}

		RepType::RepType(V v) : v(v) {}

		void RepType::deepCopy(Par<CloneEnv> env) {}

		wostream &operator <<(wostream &to, const RepType &m) {
			switch (m.v) {
			case RepType::repNone:
				to << L"repNone";
				break;
			case RepType::repZeroOne:
				to << L"repZeroOne";
				break;
			case RepType::repOnePlus:
				to << L"repOnePlus";
				break;
			case RepType::repZeroPlus:
				to << L"repZeroPlus";
				break;
			}

			return to;
		}

		Str *toS(EnginePtr e, RepType m) {
			return CREATE(Str, e.v, ::toS(m));
		}

		RepType repNone() { return RepType(RepType::repNone); }
		RepType repZeroOne() { return RepType(RepType::repZeroOne); }
		RepType repOnePlus() { return RepType(RepType::repOnePlus); }
		RepType repZeroPlus() { return RepType(RepType::repZeroPlus); }


	}
}
