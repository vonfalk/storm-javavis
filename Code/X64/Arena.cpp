#include "stdafx.h"
#include "Arena.h"
#include "Output.h"
#include "Asm.h"
#include "AsmOut.h"
#include "RemoveInvalid.h"
#include "Layout.h"

namespace code {
	namespace x64 {

		Arena::Arena() {}

		Listing *Arena::transform(Listing *l, Binary *owner) const {
			// Remove unsupported OP-codes, replacing them with their equivalents.
			l = code::transform(l, this, new (this) RemoveInvalid());

			// Expand variables and function calls as well as function prolog and epilog.
			l = code::transform(l, this, new (this) Layout(owner));

			return l;
		}

		void Arena::output(Listing *src, Output *to) const {
			code::x64::output(src, to);
		}

		LabelOutput *Arena::labelOutput() const {
			return new (this) LabelOutput(8);
		}

		CodeOutput *Arena::codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const {
			return new (this) CodeOut(owner, offsets, size, refs);
		}

		void Arena::removeFnRegs(RegSet *from) const {
			RegSet *r = fnDirtyRegs(engine());
			for (RegSet::Iter i = r->begin(); i != r->end(); ++i)
				from->remove(i.v());
		}

	}
}
