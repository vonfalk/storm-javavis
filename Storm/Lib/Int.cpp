#include "stdafx.h"
#include "Int.h"

namespace storm {

	class IntType : public Type {
	public:
		IntType() : Type(L"Int", typeValue) {}

		virtual nat size() const {
			return sizeof(code::Int);
		}
	};

	Type *intType() {
		return new IntType();
	}

}
