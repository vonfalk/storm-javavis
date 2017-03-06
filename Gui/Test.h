#pragma once
#include "Core/Array.h"
#include "Core/EnginePtr.h"

namespace gui {

	Int STORM_FN test(Int v);
	Int STORM_FN test(Str *v);
	Int STORM_FN test(Array<Int> *v);
	Thread *STORM_FN testThread(EnginePtr e, Int v);

}
