#include "stdafx.h"
#include "Debug.h"

namespace storm {
	namespace debug {

		DtorClass::DtorClass(int v) : value(v) {}

		DtorClass::~DtorClass() {
			PVAR(value);
		}

		PtrKey::PtrKey() {
			reset();
		}

		void PtrKey::reset() {
			oldPos = size_t(this);
		}

		bool PtrKey::moved() const {
			return size_t(this) != oldPos;
		}

		Extend::Extend(Int v) : v(v) {}

		Int Extend::value() {
			return v;
		}
	}
}

