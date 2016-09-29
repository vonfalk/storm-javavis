#pragma once
#include "Core/TObject.h"
#include "Core/GcType.h"
#include "Arena.h"
#include "Listing.h"
#include "RefSource.h"
#include "StackFrame.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A Binary represents a Listing that has been translated into machine code, along with any
	 * extra information needed (such as descriptions of exception handlers or similar).
	 *
	 * Note: The Binary class expects backends to create a table of data starting in the meta()
	 * where each element looks like this:
	 *   size_t freeFn; - a pointer to the function to be executed when this variable is to be freed.
	 *   size_t offset; - a signed offset to the variable relative to the stack frame provided by the backend.
	 * There shall be one entry for each variable in the Listing. Variable 0 shall be at index 0.
	 */
	class Binary : public Content {
		STORM_CLASS;
	public:
		// Translate a listing into machine code.
		STORM_CTOR Binary(Arena *arena, Listing *src);

		// Clean up a stack frame from this function.
		void cleanup(StackFrame &frame);

	private:
		// Information about a single variable.
		struct Var {
			Nat id;
			FreeOpt freeOpt;
			Size size;
		};

		// Remember the part hierarchy. These are allocated as GcArray:s to reduce the number of allocations.
		struct Part {
			// Number of elements.
			const size_t count;

			// Previous part.
			size_t prev;

			// Variables in here.
			Var vars[1];
		};

		// Offset of the metadata label.
		Nat metaOffset;

		// All parts.
		GcArray<Part *> *parts;

		// Fill in the 'parts' array.
		void fillParts(Listing *src);

		// Clean a single variable.
		void cleanup(StackFrame &frame, Var &v);

		// Type declarations for the GC.
		static const GcType partArrayType;
		static const GcType partType;
	};

}
