#pragma once
#include "Shared/LibData.h"
#include "Core/Str.h"

namespace testlib {

	class GlobalData : public storm::Object {
		STORM_CLASS;
	public:
		Int counter;

		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Get the 'global' object.
	GlobalData *STORM_FN global(EnginePtr e);

	// Get the one and only instance of 'GlobalData'.
	GlobalData *&globalData(Engine &e);

}
