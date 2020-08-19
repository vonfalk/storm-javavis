#pragma once
#include "../Output.h"
#include "Core/GcCode.h"

namespace code {
	class Binary;

	namespace x86 {
		STORM_PKG(core.asm.x86);

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

			virtual void *codePtr() const;
		protected:
			virtual void STORM_FN markLabel(Nat id);
			virtual void STORM_FN markGcRef(Ref ref);
			virtual void STORM_FN markRef(OffsetRef ref, Bool ptr);
			virtual Nat STORM_FN labelOffset(Nat id);
			virtual Nat STORM_FN toRelative(Nat id);

		private:
			// Our owner. Used to create proper references.
			Binary *owner;

			// Updaters in the code.
			Array<TObject *> *codeRefs;

			// Code we're writing to.
			UNKNOWN(PTR_GC) byte *code;

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
