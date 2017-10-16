#include "stdafx.h"
#include "Arena.h"
#include "Output.h"
#include "Listing.h"
#include "Remove64.h"
#include "RemoveInvalid.h"
#include "LayoutVars.h"
#include "Asm.h"
#include "AsmOut.h"

namespace code {
	namespace x86 {

		static bool has64(Listing *in) {
			for (nat i = 0; i < in->count(); i++) {
				if (in->at(i)->size() == Size::sLong)
					return true;
			}
			return false;
		}

		Arena::Arena() {}

		Listing *Arena::transform(Listing *l, Binary *owner) const {
			if (has64(l)) {
				// Replace any 64-bit operations with 32-bit corresponding operations.
				l = code::transform(l, this, new (this) Remove64());
			}

			// Transform any unsupported op-codes into sequences of other op-codes. Eg. referencing
			// memory twice or similar.
			l = code::transform(l, this, new (this) RemoveInvalid());

			// Expand variables and function calls as well as function prolog and epilog. We need to
			// know all used registers for this to work, so it has to be run after the previous
			// transforms.
			l = code::transform(l, this, new (this) LayoutVars(owner));

			return l;
		}

		void Arena::output(Listing *src, Output *to) const {
			code::x86::output(src, to);
		}

		LabelOutput *Arena::labelOutput() const {
			return new (this) LabelOutput(4);
		}

		CodeOutput *Arena::codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const {
			return new (this) CodeOut(owner, offsets, size, refs);
		}

		void Arena::removeFnRegs(RegSet *from) const {
			code::Arena::removeFnRegs(from);
			from->remove(ptrD);
			// esi, edi (and actually ebx as well) are preserved.
		}

		Listing *Arena::redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param) {
			Listing *l = new (this) Listing(this);

			TODO(L"Handle 'member' properly!");

			// Add parameters. We only want to free them if we get an exception.
			for (Nat i = 0; i < params->count(); i++)
				l->createParam(params->at(i), freeOnException | freePtr);

			// Output the function.
			*l << prolog();

			if (param.empty())
				*l << fnParam(ptrDesc(engine()), param);
			*l << fnCall(fn, ptrDesc(engine()), ptrA);

			*l << epilog(); // preserves ptrA
			*l << jmp(ptrA);

			return l;
		}

	}
}
