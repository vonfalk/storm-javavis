#pragma once
#include "../Output.h"
#include "Core/GcCode.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		class CodeOut : public CodeOutput {
			STORM_CLASS;
		public:
			STORM_CTOR CodeOut(Array<Nat> *lbls, Nat size, Nat refs);

			virtual void STORM_FN putByte(Byte b);
			virtual void STORM_FN putInt(Nat w);
			virtual void STORM_FN putLong(Word w);
			virtual void STORM_FN putPtr(Word w);
			virtual void STORM_FN putGcPtr(Word w);
			virtual void STORM_FN putGcRelPtr(Word w, Nat offset);
			virtual Nat STORM_FN tell() const;

			virtual void *codePtr() const;
		protected:
			virtual void STORM_FN markLabel(Nat id);
			virtual Nat STORM_FN labelOffset(Nat id);
			virtual Nat STORM_FN toRelative(Nat id);

		private:
			// Code we're writing to.
			UNKNOWN(PTR_GC) byte *code;

			// Position in the code.
			Nat pos;

			// Last used ref.
			Nat ref;

			// Label offsets, computed from before.
			Array<Nat> *labels;
		};

	}
}
