#pragma once
#include "Core/Object.h"
#include "IndentType.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Information about indentation in an InfoNode. This is only needed in the minority of
		 * nodes, so it is only created when it is actually needed.
		 */
		class InfoIndent : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoIndent(Nat start, Nat end, IndentType type);

			// Start and end position of the intended range.
			Nat start;
			Nat end;

			// Type of indentation.
			IndentType type;

			// Is 'i' included in the range?
			Bool STORM_FN contains(Nat i) const;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Represents possible results from an indent-query. Either: 'n levels of indentation' or
		 * 'same indentation as line on offset n'.
		 */
		class TextIndent {
			STORM_VALUE;
		public:
			// Create, zero levels of indentation.
			STORM_CTOR TextIndent();

			// Is this an alignment indentation?
			Bool STORM_FN isAlign() const;

			// Access the indentation level. Returns zero if we're containing a character offset.
			Nat STORM_FN level() const;
			void STORM_FN level(Nat level);

			// Access the current offset. Returns zero if we're not containing a character offset.
			Nat STORM_FN alignAs() const;
			void STORM_FN alignAs(Nat offset);

			// Apply indentation of a parent node, assuming it is applicable to us.
			void STORM_FN applyParent(InfoIndent *info, Nat offset);

			// Increase any offsets in this structure by 'n'.
			void STORM_FN offset(Nat n);

			// Compare.
			inline Bool STORM_FN operator ==(TextIndent o) const { return value == o.value; }
			inline Bool STORM_FN operator !=(TextIndent o) const { return value != o.value; }

		private:
			// Store the result. Topmost bit is set if we contain an offset into the file.
			Nat value;

			// Masks.
			static const Nat relativeMask = 0x80000000;
		};

		// Output.
		StrBuf &operator <<(StrBuf &to, TextIndent i);
		wostream &operator <<(wostream &to, TextIndent i);

		// Create initialized TextIndent instances.
		TextIndent STORM_FN indentLevel(Nat level);
		TextIndent STORM_FN indentAs(Nat offset);

	}
}
