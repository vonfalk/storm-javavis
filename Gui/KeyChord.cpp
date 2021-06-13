#include "stdafx.h"
#include "KeyChord.h"

namespace gui {

	StrBuf &operator <<(StrBuf &to, KeyChord a) {
		if (a.modifiers)
			to << name(to.engine(), a.modifiers) << S("+");
		to << c_name(a.key);
		return to;
	}

}
