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
			refs->refs[ref].param = 0;
			refs->refs[ref].kind = GcCodeRef::rawPtr;
			ref++;

			putPtr(w);
		}

		void CodeOut::putGcRelPtr(Word w, Nat offset) {
			GcCode *refs = runtime::codeRefs(code);
			refs->refs[ref].offset = pos;
			refs->refs[ref].param = offset;
			refs->refs[ref].kind = GcCodeRef::offsetPtr;
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

		Nat CodeOut::toRelative(Nat id) {
			TODO(L"Implement me!");
			return 0;
		}

	}
}
