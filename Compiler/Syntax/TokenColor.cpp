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
			{ tNone, L"<none>" },
			{ tComment, L"comment" },
			{ tDelimiter, L"delimiter" },
			{ tString, L"string" },
			{ tConstant, L"constant" },
			{ tKeyword, L"keyword" },
			{ tFnName, L"fnName" },
			{ tVarName, L"varName" },
			{ tTypeName, L"typeName" },
		};


		Str *STORM_FN name(EnginePtr e, TokenColor c) {
			for (nat i = 0; i < ARRAY_COUNT(tcTable); i++)
				if (tcTable[i].color == c)
					return new (e.v) Str(tcTable[i].name);

			return new (e.v) Str(L"<unknown>");
		}

		TokenColor STORM_FN tokenColor(Str *name) {
			for (nat i = 0; i < ARRAY_COUNT(tcTable); i++)
				if (wcscmp(tcTable[i].name, name->c_str()) == 0)
					return tcTable[i].color;

			return tNone;
		}

	}
}
