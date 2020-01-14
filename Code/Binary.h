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

		// Output the transformed ASM code for debugging.
		Binary(Arena *arena, Listing *src, Bool debug);

		// Clean up a stack frame from this function.
		void cleanup(StackFrame &frame);

		// Check if we have any catch-clauses at all (used for pre-screening in exception handlers).
		inline bool hasCatch() const { return tryParts != 0; }

		// Information on how to resume from a try-clause.
		struct Resume {
			// Next instruction to execute. 'null' if nothing is to be done.
			void *ip;

			// Stack depth.
			size_t stackDepth;

			// Part outside the one handling the exception.
			size_t cleanUntil;
		};

		// Check if we have a catch-clause for the active part or any of its parents.
		bool hasCatch(Nat part, RootObject *exception, Resume &resume);

		// Output to string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Information about a single variable.
		struct Variable {
			Nat id;

			// The low 16 bits contain FreeOpt, the high 16 bits contain the size of the variable.
			Nat flags;

			enum {
				sByte = 0x1000,
				sInt = 0x2000,
				sLong = 0x3000,
				sPtr = 0x4000,
				sMask = 0xFF000
			};
		};

		// Remember the part hierarchy. These are allocated as GcArray:s to reduce the number of allocations.
		struct Part {
			// Number of elements.
			const size_t count;

			// Previous part.
			size_t prev;

			// Variables in here.
			Variable vars[1];
		};

		// Information for try-blocks.
		struct TryInfo {
			// Current part id.
			Nat partId;

			// Offset to continue execution from.
			Nat resumeOffset;

			// Type to catch.
			Type *type;
		};

		// Offset of the metadata label.
		Nat metaOffset;

		// All parts.
		GcArray<Part *> *parts;

		// Try-block information. 'null' if no exceptions are caught. Indices do not correspond to
		// parts here since the array is sparse. It is necessary to perform search the array
		// (perhaps using a binary search).
		GcArray<TryInfo> *tryParts;

		// Compile the Listing object.
		void compile(Arena *arena, Listing *src, Bool debug);

		// Fill the 'parts' array.
		void fillParts(Listing *src);

		// Fill the 'tryParts' array if necessary.
		void fillTryParts(Listing *src, LabelOutput *labels);

		// Find a try-part corresponding to a given part. Returns null if none exists.
		TryInfo *findTryInfo(Nat part);

		// Clean a single variable.
		void cleanup(StackFrame &frame, Variable &v);

		// Type declarations for the GC.
		static const GcType partArrayType;
		static const GcType partType;
		static const GcType tryInfoArrayType;
	};

}
