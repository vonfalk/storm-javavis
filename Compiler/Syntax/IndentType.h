#pragma once

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Kind of indentation for a production.
		 */
		enum IndentType {
			indentNone,

			// Increase indentation one level.
			indentIncrease,

			// Decrease indentation one level.
			indentDecrease,

			// Align with the start of the indented range. Only the leafmost alignment is
			// considered when indentation information is generated.
			indentAlign,
		};

		// Output.
		StrBuf &STORM_FN operator <<(StrBuf &to, IndentType i);

	}
}
