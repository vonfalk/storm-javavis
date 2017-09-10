#include "stdafx.h"
#include "Layout.h"

namespace code {
	namespace x64 {

#define TRANSFORM(x) { op::x, &Layout::x ## Tfm }

		const OpEntry<Layout::TransformFn> Layout::transformMap[] = {
			TRANSFORM(prolog),
			TRANSFORM(epilog),
			TRANSFORM(beginBlock),
			TRANSFORM(endBlock),
		};


		Layout::Layout(Binary *owner) : owner(owner) {}

		void Layout::before(Listing *dest, Listing *src) {
			UNUSED(dest);
			UNUSED(src);
			// TODO!
		}

		void Layout::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, src, line);
			} else {
				*dest << i->alter(resolve(src, i->dest()), resolve(src, i->src()));
			}
		}

		void Layout::after(Listing *dest, Listing *src) {
			*dest << dest->meta();
			// TODO: Output metadata table.
		}

		Operand Layout::resolve(Listing *src, const Operand &op) {
			UNUSED(src);
			// TODO!
			return op;
		}

		void Layout::prologTfm(Listing *dest, Listing *src, Nat line) {
			*dest << push(ptrFrame);
			*dest << mov(ptrFrame, ptrStack);

			// TODO: Finish!
		}

		void Layout::epilogTfm(Listing *dest, Listing *src, Nat line) {
			// TODO: Finish!

			*dest << mov(ptrStack, ptrFrame);
			*dest << pop(ptrFrame);
		}

		void Layout::beginBlockTfm(Listing *dest, Listing *src, Nat line) {}

		void Layout::endBlockTfm(Listing *dest, Listing *src, Nat line) {}

	}
}
