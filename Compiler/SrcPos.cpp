#include "stdafx.h"
#include "SrcPos.h"

namespace storm {

	void SrcPos::deepCopy(CloneEnv *env) {}

	wostream &operator <<(wostream &to, const SrcPos &p) {
		return to << L"SrcPos";
	}

}
