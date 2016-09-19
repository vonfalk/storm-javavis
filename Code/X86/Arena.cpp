#include "stdafx.h"
#include "Arena.h"
#include "Output.h"
#include "Listing.h"
#include "Layout.h"
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
			l = code::transform(l, this, new (this) LayoutVars());

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

		static Offset paramOffset(Listing *src, Variable var) {
			if (var == Variable()) {
				// Old ebp and return pointer.
				return Offset::sPtr * 2;
			}

			Variable prev = src->prev(var);
			Offset offset = paramOffset(src, prev) + prev.size();
			return offset.alignAs(Size::sPtr);
		}

		Array<Offset> *layout(Listing *src, Nat savedRegs, Bool usingEH) {
			Array<Offset> *result = code::layout(src);
			Array<Variable> *all = src->allVars();

			Offset varOffset;
			// Exception handler frame.
			if (usingEH)
				varOffset += Size::sPtr * 4;
			// Saved registers.
			varOffset += Size::sPtr * savedRegs;

			for (nat i = 0; i < all->count(); i++) {
				Variable var = all->at(i);
				Nat id = var.key();

				if (src->isParam(var)) {
					result->at(id) = paramOffset(src, var);
				} else {
					result->at(id) = -(result->at(id) + var.size() + varOffset);
				}
			}

			result->last() = result->last() + varOffset;
			return result;
		}

	}
}
