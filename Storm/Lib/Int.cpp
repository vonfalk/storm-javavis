#include "stdafx.h"
#include "Int.h"

namespace storm {

	class IntType : public Type {
	public:
		IntType(Engine &e) : Type(e, L"Int", typeValue) {}

		virtual nat size() const {
			return sizeof(code::Int);
		}
	};

	Type *intType(Engine &e) {
		return new IntType(e);
	}


	class NatType : public Type {
	public:
		NatType(Engine &e) : Type(e, L"Nat", typeValue) {}

		virtual nat size() const {
			return sizeof(code::Nat);
		}
	};

	Type *natType(Engine &e) {
		return new NatType(e);
	}

}
