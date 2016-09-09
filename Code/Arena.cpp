#include "stdafx.h"
#include "Arena.h"
#include "X86/Arena.h"

namespace code {

	Arena::Arena() {}

	Listing *Arena::transform(Listing *src, Binary *owner) const {
		assert(false);
		return src;
	}

	void Arena::output(Listing *src, Output *to) const {
		assert(false);
	}

	LabelOutput *Arena::labelOutput() const {
		assert(false);
		return null;
	}

	CodeOutput *Arena::codeOutput(Array<Nat> *offsets, Nat size, Nat refs) const {
		assert(false);
		return null;
	}

	CodeOutput *Arena::codeOutput(LabelOutput *src) const {
		return codeOutput(src->offsets, src->size, src->refs);
	}

#if defined(X86) && defined(WINDOWS)
	Arena *arena(EnginePtr e) {
		return new (e.v) x86::Arena();
	}
#endif
}
