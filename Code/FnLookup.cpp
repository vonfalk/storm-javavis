#include "stdafx.h"
#include "FnLookup.h"
#include "Utils/Path.h"

namespace code {

	ArenaLookup::ArenaLookup(Arena &arena) : arena(arena) {}

	String ArenaLookup::format(const StackFrame &frame) const {
		TODO(L"Implement me!");
		return CppLookup::format(frame);
	}

}
