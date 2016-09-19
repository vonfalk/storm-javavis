#include "stdafx.h"
#include "Output.h"
#include "Binary.h"
#include "Utils/Bitwise.h"

namespace code {
	namespace x86 {

		CodeOut::CodeOut(Binary *owner, Array<Nat> *lbls, Nat size, Nat numRefs) {
			// Properly align 'size'.
			size = roundUp(size, sizeof(void *));

			// Initialize our members.
			this->owner = owner;
			codeRefs = new (this) Array<Reference *>();
			code = (byte *)runtime::allocCode(engine(), size + sizeof(void *), numRefs + 1);
			labels = lbls;
			pos = 0;
			ref = 1;

			// Store 'codeRefs' at the end of our allocated space.
			void *refsPos = code + size;
			GcCode *refs = runtime::codeRefs(code);
			refs->refs[0].offset = size;
			refs->refs[0].kind = GcCodeRef::rawPtr;
			*(void **)refsPos = codeRefs;
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

		void CodeOut::markGcRef(Ref r) {
			if (ref == 0)
				return;

			codeRefs->push(new (this) CodeUpdater(r, owner, code, ref - 1));
		}

		Nat CodeOut::labelOffset(Nat id) {
			if (id < labels->count()) {
				return labels->at(id);
			} else {
				assert(false, L"Unknown label id: " + ::toS(id));
				return 0;
			}
		}

		Nat CodeOut::toRelative(Nat offset) {
			return offset - (pos + 4);
		}

	}
}
