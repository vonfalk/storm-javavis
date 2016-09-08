#include "stdafx.h"
#include "Output.h"

namespace code {
	namespace x86 {

		CodeOut::CodeOut(Array<Nat> *lbls, Nat size) {
			TODO(L"Properly include references as well!");
			code = (byte *)runtime::allocCode(engine(), size, 0);
			pos = 0;
		}

		void CodeOut::putByte(Byte b) {
			code[pos++] = b;
		}

		void CodeOut::putInt(Nat w) {
			Nat *to = (Nat *)&code[pos];
			*to = w;
			pos += 4;
		}

		void CodeOut::putLong(Word w) {
			Word *to = (Word *)&code[pos];
			*to = w;
			pos += 8;
		}

		void CodeOut::putPtr(Word w) {
			Nat *to = (Nat *)&code[pos];
			*to = (Nat)w;
			pos += 4;
		}

		Nat CodeOut::tell() const {
			return pos;
		}

		void *CodeOut::codePtr() const {
			return code;
		}

		void CodeOut::markLabel(Nat id) {
			// No need. This should already be done for us.
		}

		Nat CodeOut::labelOffset(Nat id) {
			if (id < labels->count())
				return labels->at(id);
			else
				return 0;
		}

		Nat CodeOut::toRelative(Nat id) {
			TODO(L"Implement me!");
			return 0;
		}

	}
}
