#pragma once
#include "Type.h"

namespace storm {

	class BoolType : public Type {
		STORM_CLASS;
	public:
		BoolType() : Type(L"Bool", typeValue) {}

		virtual nat size() const {
			return sizeof(storm::Bool);
		}

		virtual bool isBuiltIn() const { return true; }
		virtual code::Value destructor() const { return code::Value(); }
	};



}
