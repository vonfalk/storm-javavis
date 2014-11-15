#pragma once
#include "Type.h"

namespace storm {

	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType(Engine &e) : Type(e, L"Bool", typeValue) {}

		virtual nat size() const {
			return sizeof(storm::Bool);
		}
	};



}
