#pragma once
#include "Utils/Exception.h"
#include "Core/Exception.h"
#include "Core/SrcPos.h"
#include "Value.h"
#include "ExprResult.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Defines exceptions used by the compiler.
	 */
	class EXCEPTION_EXPORT CodeError : public Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR CodeError(SrcPos where) {
			pos = where;
		}

		// Where is the error located?
		SrcPos pos;

		// Message.
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("@") << pos << S(": ");
			messageText(to);
		}

		// Get only the text of the error, no position information.
		virtual void STORM_FN messageText(StrBuf *to) const ABSTRACT;

		// Get the error as a string.
		Str *STORM_FN messageText() const;
	};


	/**
	 * Language definition error.
	 */
	class EXCEPTION_EXPORT LangDefError : public Exception {
		STORM_EXCEPTION;
	public:
		LangDefError(const wchar *msg) {
			w = new (this) Str(msg);
		}
		STORM_CTOR LangDefError(Str *msg) {
			w = msg;
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << w;
		}
	private:
		Str *w;
	};

	/**
	 * Specific subclass when calling core:debug:throwError.
	 */
	class EXCEPTION_EXPORT DebugError : public Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR DebugError() {
			saveTrace();
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Debug error");
		}
	};


	/**
	 * Internal type error (something in C++ went wrong).
	 */
	class EXCEPTION_EXPORT InternalTypeError : public InternalError {
		STORM_EXCEPTION;
	public:
		InternalTypeError(const wchar *context, Type *expected, Type *got);
		STORM_CTOR InternalTypeError(Str *context, Type *expected, Type *got);
	};


	/**
	 * Some syntax error.
	 */
	class EXCEPTION_EXPORT SyntaxError : public CodeError {
		STORM_EXCEPTION;
	public:
		SyntaxError(SrcPos where, const wchar *msg);
		STORM_CTOR SyntaxError(SrcPos where, Str *msg);

		virtual void STORM_FN messageText(StrBuf *to) const {
			*to << S("Syntax error: ") << text;
		}

		Str *text;
	};


	/**
	 * Type checking error.
	 */
	class EXCEPTION_EXPORT TypeError : public CodeError {
		STORM_EXCEPTION;
	public:
		TypeError(SrcPos where, const wchar *msg);
		STORM_CTOR TypeError(SrcPos where, Str *msg);
		STORM_CTOR TypeError(SrcPos where, Value expected, ExprResult got);
		STORM_CTOR TypeError(SrcPos where, Value expected, Value got);

		virtual void STORM_FN messageText(StrBuf *to) const {
			*to << S("Type error: ") << msg;
		}

	private:
		Str *msg;
	};

	/**
	 * Error in type definitions.
	 */
	class EXCEPTION_EXPORT TypedefError : public CodeError {
		STORM_EXCEPTION;
	public:
		TypedefError(SrcPos pos, const wchar *msg);
		STORM_CTOR TypedefError(SrcPos pos, Str *msg);

		virtual void STORM_FN messageText(StrBuf *to) const {
			*to << S("Type definition error: ") << msg;
		}

	private:
		Str *msg;
	};


	/**
	 * Error while handling built-in functions.
	 */
	class EXCEPTION_EXPORT BuiltInError : public Exception {
		STORM_EXCEPTION;
	public:
		BuiltInError(const wchar *msg) {
			this->msg = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR BuiltInError(Str *msg) {
			this->msg = msg;
			saveTrace();
		}
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Error while loading built in functions: ") << msg;
		}
	private:
		Str *msg;
	};


	/**
	 * Trying to instantiate an abstract class.
	 */
	class EXCEPTION_EXPORT InstantiationError : public CodeError {
		STORM_EXCEPTION;
	public:
		InstantiationError(SrcPos pos, const wchar *msg);
		InstantiationError(SrcPos pos, Str *msg);
		virtual void STORM_FN messageText(StrBuf *to) const {
			*to << S("Instantiation error: ") << msg;
		}
	private:
		Str *msg;
	};

}
