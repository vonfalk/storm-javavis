#include "stdafx.h"
#include "Arena.h"
#include "Output.h"
#include "AsmOut.h"
#include "Layout.h"

namespace code {
	namespace x64 {

		Arena::Arena() {}

		Listing *Arena::transform(Listing *l, Binary *owner) const {
			// TODO: We need a transform that removes:
			// - immediate values that require 64 bits
			// - references and object references (same reason as above)
			// - memory-memory operands

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
			code::Arena::removeFnRegs(from);
			TODO(L"Remove registers not preserved through function calls.");
		}

	}
}
