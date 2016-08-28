#include "stdafx.h"
#include "Arena.h"
#include "OutputX86.h"

namespace code {
	namespace x86 {

		Arena::Arena() {}

		Listing *Arena::transform(Listing *src, Binary *owner) const {
			TODO(L"Implement me!");
			return src;
		}

		void Arena::output(Listing *src, Output *to) const {
			TODO(L"Implement me!");
		}

		LabelOutput *Arena::labelOutput() const {
			return new (this) LabelOutput(4);
		}

		Output *Arena::codeOutput(Array<Nat> *offsets, Nat size) const {
			TODO(L"Implement me!");
			return null;
		}


	}
}
