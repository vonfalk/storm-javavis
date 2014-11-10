#include "stdafx.h"
#include "Bool.h"
#include "Type.h"

namespace storm {

	class BoolType : public Type {
	public:
		BoolType(Engine &e) : Type(e, L"Bool", typeValue) {}

		virtual nat size() const {
			return sizeof(storm::Bool);
		}
	};

	Type *boolType(Engine &e) {
		return new BoolType(e);
	}


}
