#include "stdafx.h"
#include "State.h"

namespace storm {
	namespace syntax {

		State::State() : from(0) {}

		State::State(ProductionIter pos, Nat from, State *prev, State *completed)
			: pos(pos), from(from), prev(prev), completed(completed) {}

		void State::toS(StrBuf *to) const {
			*to << L"{" << pos << L", " << from << L", " << hex(prev) << L", " << hex(completed) << L"}";
		}


		StateSet::StateSet() {}

	}
}
