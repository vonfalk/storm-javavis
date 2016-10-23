#include "stdafx.h"
#include "Arena.h"
#include "Reg.h"
#include "X86/Arena.h"
#include "Core/Str.h"

namespace code {

	Arena::Arena() {}

	Ref Arena::external(const wchar *name, const void *ptr) const {
		return Ref(externalSource(name, ptr));
	}

	RefSource *Arena::externalSource(const wchar *name, const void *ptr) const {
		RefSource *src = new (this) RefSource(name);
		src->setPtr(ptr);
		return src;
	}

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

	CodeOutput *Arena::codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const {
		assert(false);
		return null;
	}

	CodeOutput *Arena::codeOutput(Binary *owner, LabelOutput *src) const {
		return codeOutput(owner, src->offsets, src->size, src->refs);
	}

	void Arena::removeFnRegs(RegSet *from) const {
		from->remove(ptrA);
		from->remove(ptrB);
		from->remove(ptrC);
	}

#if defined(X86) && defined(WINDOWS)
	Arena *arena(EnginePtr e) {
		return new (e.v) x86::Arena();
	}
#endif
}
