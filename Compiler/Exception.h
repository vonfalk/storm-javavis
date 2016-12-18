#pragma once
#include "Utils/Exception.h"
#include "SrcPos.h"
#include "Value.h"
#include "ExprResult.h"

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

	protected:
		// We generally do not need a full stack trace here, so we skip that.
		virtual void output(wostream &to) const;
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
	 * Language definition error.
	 */
	class LangDefError : public Exception {
	public:
		inline LangDefError(const String &w) : w(w) {}
		inline virtual String what() const { return w; }
	private:
		String w;
	};

	/**
	 * Specific subclass when calling core:debug:throwError.
	 */
	class DebugError : public Exception {
		inline virtual String what() const { return L"Debug error"; }
	};


	/**
	 * Internal type error (something in C++ went wrong).
	 */
	class InternalTypeError : public InternalError {
	public:
		InternalTypeError(const String &context, const Type *expected, const Type *got);
	};


	/**
	 * Some syntax error.
	 */
	class SyntaxError : public CodeError {
	public:
		inline SyntaxError(const SrcPos &where, const String &msg) : CodeError(where), msg(msg) {}

		inline virtual String what() const {
			return L"@" + ::toS(where) + L": Syntax error: " + msg;
		}

	private:
		String msg;
	};


	/**
	 * Type checking error.
	 */
	class TypeError : public CodeError {
	public:
		inline TypeError(const SrcPos &where, const String &msg) : CodeError(where), msg(msg) {}
		inline TypeError(const SrcPos &where, const Value &expected, const ExprResult &got)
			: CodeError(where), msg(L"Expected " + ::toS(expected) + L" but got " + ::toS(got)) {}
		inline TypeError(const SrcPos &where, const Value &expected, const Value &got)
			: CodeError(where), msg(L"Expected " + ::toS(expected) + L" but got " + ::toS(got)) {}

		inline virtual String what() const {
			return L"@" + ::toS(where) + L": Type error: " + msg;
		}

	private:
		String msg;
	};

	/**
	 * Error in type definitions.
	 * TODO: Require a SrcPos!
	 */
	class TypedefError : public CodeError {
	public:
		inline TypedefError(const String &msg) : CodeError(SrcPos()), msg(msg) { TODO("Require a SrcPos!"); }
		inline virtual String what() const {
			return L"@" + ::toS(where) + L": Type definition error: " + msg;
		}
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
