#include "stdafx.h"
#include "Arena.h"
#include "OutputX86.h"
#include "Listing.h"
#include "RemoveInvalid.h"

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
				TODO(L"We need to transform the 64-bit ops!");
			}

			// Transform any unsupported op-codes into sequences of other op-codes. Eg. referencing
			// memory twice or similar.
			l = code::transform(l, new (this) RemoveInvalid());

			// Expand variables and function calls as well as function prolog and epilog. We need to
			// know all used registers for this to work, so it has to be run after the previous
			// transforms.

			TODO(L"Implement me!");
			return l;
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

		const Register ptrD = Register(0x010);
		const Register ptrSi = Register(0x011);
		const Register ptrDi = Register(0x012);
		const Register edx = Register(0x410);
		const Register esi = Register(0x411);
		const Register edi = Register(0x412);

		const wchar *nameX86(Register r) {
			switch (r) {
			case ptrD:
				return L"ptrD";
			case ptrSi:
				return L"ptrSi";
			case ptrDi:
				return L"ptrDi";

			case edx:
				return L"edx";
			case esi:
				return L"esi";
			case edi:
				return L"edi";
			}
			return null;
		}

	}
}
