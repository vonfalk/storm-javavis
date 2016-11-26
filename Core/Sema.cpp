#include "stdafx.h"
#include "Sema.h"

namespace storm {

	Sema::Sema() : alloc(new Data(1)) {}

	Sema::Sema(Nat count) : alloc(new Data(count)) {}

	Sema::Sema(const Sema &o) : alloc(o.alloc) {
		atomicIncrement(alloc->refs);
	}

	Sema::~Sema() {
		if (atomicDecrement(alloc->refs) == 0) {
			// Last one remaining.
			delete alloc;
		}
	}

	void Sema::deepCopy(CloneEnv *) {
		// Nothing to do.
	}

	void Sema::up() {
		alloc->sema.up();
	}

	void Sema::down() {
		alloc->sema.down();
	}

	Sema::Data::Data(Nat count) : refs(1), sema(count) {}

}
