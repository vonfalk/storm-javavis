#pragma once
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Output.h"

namespace code {
	STORM_PKG(core.asm);

	class Listing;
	class Binary;

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
		virtual CodeOutput *STORM_FN codeOutput(Array<Nat> *lblOffsets, Nat size, Nat refs) const;
		CodeOutput *STORM_FN codeOutput(LabelOutput *src) const;

	};

	// Create an arena for this platform.
	Arena *STORM_FN arena(EnginePtr e);
}
