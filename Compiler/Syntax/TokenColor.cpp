#include "stdafx.h"
#include "TokenColor.h"
#include "Core/Str.h"

namespace storm {
	namespace syntax {

		struct TCEntry {
			TokenColor color;
			const wchar *name;
		};

		static const TCEntry tcTable[] = {
			{ tNone, S("none") },
			{ tComment, S("comment") },
			{ tDelimiter, S("delimiter") },
			{ tString, S("string") },
			{ tConstant, S("constant") },
			{ tKeyword, S("keyword") },
			{ tFnName, S("fnName") },
			{ tVarName, S("varName") },
			{ tTypeName, S("typeName") },
		};


		Str *STORM_FN name(EnginePtr e, TokenColor c) {
			for (nat i = 0; i < ARRAY_COUNT(tcTable); i++)
				if (tcTable[i].color == c)
					return new (e.v) Str(tcTable[i].name);

			return new (e.v) Str(S("<unknown>"));
		}

		TokenColor STORM_FN tokenColor(Str *name) {
			for (nat i = 0; i < ARRAY_COUNT(tcTable); i++)
				if (*name == tcTable[i].name)
					return tcTable[i].color;

			return tNone;
		}

	}
}
