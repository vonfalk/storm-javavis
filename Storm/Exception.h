#pragma once
#include "Utils/Exception.h"

#include "SrcPos.h"

namespace storm {

	/**
	 * Runtime errors.
	 */

	class RuntimeError : public Exception {
	public:
		inline RuntimeError(const String &w) : w(w) {}
		inline virtual String what() const { return w; }
	private:
		String w;
	};


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
	 * Internal error.
	 */
	class InternalError : public Exception {
	public:
		inline InternalError(const String &w) : w(w) {}
		inline virtual String what() const { return w; }
	private:
		String w;
	};


	/**
	 * Some syntax error.
	 */
	class SyntaxError : public CodeError {
	public:
		inline SyntaxError(const SrcPos &where, const String &msg) : CodeError(where), msg(msg) {}

		inline virtual String what() const {
			return toS(where) + L": Syntax error: " + msg;
		}

	private:
		String msg;
	};


	/**
	 * Error in type definitions.
	 */
	class TypedefError : public CodeError {
	public:
		inline TypedefError(const String &msg) : CodeError(SrcPos()), msg(msg) { TODO("Require a SrcPos!"); }
		inline virtual String what() const { return toS(where) + L": Type definition error: " + msg; }
	private:
		String msg;
	};


	/**
	 * Error while handling built-in functions.
	 */
	class BuiltInError : public Exception {
	public:
		BuiltInError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Error while loading built in functions: " + msg; }
	private:
		String msg;
	};


}
