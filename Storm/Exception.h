#pragma once
#include "Utils/Exception.h"

#include "SrcPos.h"

namespace storm {

	/**
	 * Defines exceptions used in the compiler.
	 */

	class CodeError : public Exception {
	public:
		inline CodeError(const SrcPos &where) : where(where) {}

		// Where is the error located?
		SrcPos where;
	};


	/**
	 * Some syntax error.
	 */
	class SyntaxError : public CodeError {
	public:
		inline SyntaxError(const SrcPos &where, const String &msg) : CodeError(where), msg(msg) {}

		inline virtual String what() const {
			return where.toS() + L": Syntax error: " + msg;
		}

	private:
		String msg;
	};

}
