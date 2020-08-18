#pragma once
#include "../Output.h"
#include "Core/GcCode.h"
#include "DwarfEh.h"

namespace code {
	class Binary;
	namespace x64 {
		STORM_PKG(core.asm.x64);

		class CodeOut : public CodeOutput {
			STORM_CLASS;
		public:
			STORM_CTOR CodeOut(Binary *owner, Array<Nat> *lbls, Nat size, Nat refs);

			virtual void STORM_FN putByte(Byte b);
			virtual void STORM_FN putInt(Nat w);
			virtual void STORM_FN putLong(Word w);
			virtual void STORM_FN putPtr(Word w);
			virtual void STORM_FN align(Nat to);
			virtual void putGc(GcCodeRef::Kind kind, Nat size, Word w);
			virtual void STORM_FN putGcPtr(Word w);
			virtual void STORM_FN putGcRelative(Word w);
			virtual void STORM_FN putRelativeStatic(Word w);
			virtual void STORM_FN putPtrSelf(Word w);
			virtual Nat STORM_FN tell() const;

			virtual void STORM_FN markProlog();
			virtual void STORM_FN markEpilog();
			virtual void STORM_FN markSaved(Reg reg, Offset offset);

			virtual void *codePtr() const;
		protected:
			virtual void STORM_FN markLabel(Nat id);
			virtual void STORM_FN markGcRef(Ref ref);
			virtual void STORM_FN markRef(Ref ref, Nat offset);
			virtual Nat STORM_FN labelOffset(Nat id);
			virtual Nat STORM_FN toRelative(Nat id);

		private:
			// Our owner. Used to create proper references.
			Binary *owner;

			// Updaters in the code.
			Array<Reference *> *codeRefs;

			// Code we're writing to.
			UNKNOWN(PTR_GC) byte *code;

			// CFI object we're writing to.
			FnInfo fnInfo;

			// Position in the code.
			Nat pos;

			// Size of the code.
			Nat size;

			// First free ref.
			Nat ref;

			// Label offsets, computed from before.
			Array<Nat> *labels;
		};

	}
}
