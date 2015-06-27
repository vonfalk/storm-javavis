#include "stdafx.h"
#include "Long.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"
#include "CodeGen.h"

namespace storm {

	LongType::LongType() : Type(L"Long", typeValue | typeFinal, Size::sInt, null) {}

	bool LongType::loadAll() {
		// TODO!
		return Type::loadAll();
	}

	Type *longType(Engine &e) {
		Type *t = e.specialBuiltIn(specialLong);
		if (!t) {
			t = CREATE(LongType, e);
			e.setSpecialBuiltIn(specialLong, t);
		}
		return t;
	}

	WordType::WordType() : Type(L"Word", typeValue | typeFinal, Size::sInt, null) {}

	bool WordType::loadAll() {
		// TODO!
		return Type::loadAll();
	}

	Type *wordType(Engine &e) {
		Type *t = e.specialBuiltIn(specialWord);
		if (!t) {
			t = CREATE(WordType, e);
			e.setSpecialBuiltIn(specialWord, t);
		}
		return t;
	}

	Str *toS(EnginePtr e, Long v) {
		return CREATE(Str, e.v, ::toS(v));
	}

	Str *toS(EnginePtr e, Word v) {
		return CREATE(Str, e.v, ::toS(v));
	}

}
