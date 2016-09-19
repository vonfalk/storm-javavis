#pragma once
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Output.h"

namespace code {
	STORM_PKG(core.asm);

	class Listing;
	class Binary;
	class RegSet;

	/**
	 * An arena represents a collection of compiled code and external references for some architecture.
	 *
	 * Abstract class, there is one instantiation for each supported platform.
	 */
	class Arena : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create an arena.
		Arena();

		// Create external references.
		Ref external(const wchar *name, const void *ptr) const;

		/**
		 * Transform and translate code into machine code.
		 */

		// Transform the code in preparation for this backend's code generation. This is
		// backend-specific. 'owner' is the binary object that will be called to handle exceptions.
		virtual Listing *STORM_FN transform(Listing *src, Binary *owner) const;

		// Translate a previously transformed listing into machine code for this arena.
		virtual void STORM_FN output(Listing *src, Output *to) const;

		/**
		 * Create output objects for this backend.
		 */

		// Create an offset-computing output.
		virtual LabelOutput *STORM_FN labelOutput() const;

		// Create a code-generating output. 'lblOffsets' and 'size' are obtained through 'labelOutput'.
		virtual CodeOutput *STORM_FN codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const;
		CodeOutput *STORM_FN codeOutput(Binary *owner, LabelOutput *src) const;

		// Remove all registers not preserved during a function call on this platform. This
		// implementation removes ptrA, ptrB and ptrC, but other Arena implementations may want to
		// remove others as well.
		virtual void STORM_FN removeFnRegs(RegSet *from) const;

	};

	// Create an arena for this platform.
	Arena *STORM_FN arena(EnginePtr e);
}
