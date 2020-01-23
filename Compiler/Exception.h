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
		STORM_CLASS;
	public:
		STORM_CTOR CodeError(SrcPos where) {
			this->where = where;
		}

		// Where is the error located?
		SrcPos where;

		// Message.
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("@") << where << S(": ");
		}
	};


	/**
	 * Language definition error.
	 */
	class EXCEPTION_EXPORT LangDefError : public Exception {
		STORM_CLASS;
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
		STORM_CLASS;
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
		STORM_CLASS;
	public:
		InternalTypeError(const wchar *context, Type *expected, Type *got);
		STORM_CTOR InternalTypeError(Str *context, Type *expected, Type *got);
	};


	/**
	 * Some syntax error.
	 */
	class EXCEPTION_EXPORT SyntaxError : public CodeError {
		STORM_CLASS;
	public:
		SyntaxError(SrcPos where, const wchar *msg);
		STORM_CTOR SyntaxError(SrcPos where, Str *msg);

		virtual void STORM_FN message(StrBuf *to) const {
			CodeError::message(to);
			*to << S("Syntax error: ") << msg;
		}

	private:
		Str *msg;
	};


	/**
	 * Type checking error.
	 */
	class EXCEPTION_EXPORT TypeError : public CodeError {
		STORM_CLASS;
	public:
		TypeError(SrcPos where, const wchar *msg);
		STORM_CTOR TypeError(SrcPos where, Str *msg);
		STORM_CTOR TypeError(SrcPos where, Value expected, ExprResult got);
		STORM_CTOR TypeError(SrcPos where, Value expected, Value got);

		virtual void STORM_FN message(StrBuf *to) const {
			CodeError::message(to);
			*to << S("Type error: ") << msg;
		}

	private:
		Str *msg;
	};

	/**
	 * Error in type definitions.
	 * TODO: Require a SrcPos!
	 */
	class EXCEPTION_EXPORT TypedefError : public CodeError {
		STORM_CLASS;
	public:
		TypedefError(const wchar *msg);
		STORM_CTOR TypedefError(Str *msg);

		virtual void STORM_FN message(StrBuf *to) const {
			CodeError::message(to);
			*to << S("Type definition error: ") << msg;
		}

	private:
		Str *msg;
	};


	/**
	 * Error while handling built-in functions.
	 */
	class EXCEPTION_EXPORT BuiltInError : public Exception {
		STORM_CLASS;
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
		STORM_CLASS;
	public:
		InstantiationError(SrcPos pos, const wchar *msg);
		InstantiationError(SrcPos pos, Str *msg);
		virtual void STORM_FN message(StrBuf *to) const {
			CodeError::message(to);
			*to << S("Instantiation error: ") << msg;
		}
	private:
		Str *msg;
	};

}
