#include "stdafx.h"
#include "Output.h"

namespace code {
	namespace x86 {

		CodeOut::CodeOut(Array<Nat> *lbls, Nat size, Nat refs) {
			code = (byte *)runtime::allocCode(engine(), size, refs);
			labels = lbls;
			pos = 0;
			ref = 0;
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

		void CodeOut::putGcPtr(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::rawPtr;
			ref++;

			putPtr(w);
		}

		void CodeOut::putGcRelative(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relativePtr;
			ref++;

			Nat *to = (Nat *)&code[pos];
			*to = Nat(w) - Nat(to + 1);
			pos += 4;
		}

		void CodeOut::putRelativeStatic(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::relative;
			ref++;

			Nat *to = (Nat *)&code[pos];
			*to = Nat(w) - Nat(to + 1);
			pos += 4;
		}

		void CodeOut::putPtrSelf(Word w) {
			GcCode *refs = runtime::codeRefs(code);
			refs->refs[ref].offset = pos;
			refs->refs[ref].kind = GcCodeRef::inside;
			ref++;

			putPtr(w);
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

		Nat CodeOut::toRelative(Nat offset) {
			return offset - (pos + 4);
		}

	}
}
