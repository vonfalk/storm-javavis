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

		Listing *Arena::redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param) {
			Listing *l = new (this) Listing(this);

			// Generate a layout of all parameters so we can properly restore them later.
			Params *layout = new (this) Params();
			if (code::x64::result(result)->memory)
				layout->add(Param::returnId, ptrPrimitive());
			for (Nat i = 0; i < params->count(); i++)
				layout->add(i, params->at(i));

			// Note: We want to use the 'prolog' and 'epilog' functionality so that exceptions from
			// 'fn' are able to propagate through this stub properly.
			*l << prolog();

			// Store the registers used for parameters inside variables on the stack.
			Array<Var> *vars = new (this) Array<Var>(layout->registerCount(), Var());
			for (Nat i = 0; i < layout->registerCount(); i++) {
				if (layout->registerAt(i) != Param()) {
					Var &v = vars->at(i);
					v = l->createVar(l->root(), Size::sLong);
					*l << mov(v, asSize(layout->registerSrc(i), Size::sLong));
				}
			}

			// Call 'fn' to obtain the actual function to call.
			if (!param.empty())
				*l << fnParam(ptrDesc(engine()), param);
			*l << fnCall(fn, ptrDesc(engine()), ptrA);

			// Restore the registers.
			for (Nat i = 0; i < layout->registerCount(); i++) {
				Var v = vars->at(i);
				if (v != Var())
					*l << mov(asSize(layout->registerSrc(i), Size::sLong), v);
			}

			// Note: The epilog will preserve all registers in this case since there are no destructors to call!
			*l << epilog();
			*l << jmp(ptrA);

			return l;
		}

	}
}
