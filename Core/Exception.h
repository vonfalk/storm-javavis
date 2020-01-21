#pragma once
#include "Str.h"
#include "StackTrace.h"
#include "OldException.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Base class for all exceptions in Storm.
	 *
	 * TODO: Rename to 'Exception' when the new system works properly and the old exceptions are removed.
	 */
	class EXCEPTION_EXPORT NException : public Object {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR NException();

		// Copy.
		NException(const NException &o);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get the message.
		Str *STORM_FN message() const;

		// Append the message to a string buffer.
		virtual void STORM_FN message(StrBuf *to) const ABSTRACT;

		// Collected stack trace, if any.
		StackTrace stackTrace;

	protected:
		// Regular to string implementation.
		virtual void STORM_FN toS(StrBuf *to) const;
	};


	/**
	 * Generic exceptions.
	 */
	class EXCEPTION_EXPORT NotSupported : public NException {
		STORM_CLASS;
	public:
		NotSupported(const wchar *msg) {
			this->msg = new (this) Str(msg);
		}
		STORM_CTOR NotSupported(Str *msg) {
			this->msg = msg;
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Operation not supported: ") << msg;
		}

	private:
		Str *msg;
	};


	/**
	 * Internal error.
	 */
	class EXCEPTION_EXPORT InternalError : public Exception {
		STORM_CLASS;
	public:
		InternalError(const wchar *msg) {
			this->msg = new (this) Str(msg);
		}
		STORM_CTOR InternalError(Str *msg) {
			this->msg = msg;
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};


	/**
	 * Runtime errors.
	 */
	class EXCEPTION_EXPORT RuntimeError : public Exception {
		STORM_CLASS;
	public:
		RuntimeError(const wchar *msg) {
			this->msg = new (this) Str(msg);
		}
		STORM_CTOR RuntimeError(Str *msg) {
			this->msg = msg;
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << msg;
		}
	private:
		Str *msg;
	};


	/**
	 * Calling an abstract function.
	 */
	class EXCEPTION_EXPORT AbstractFnCalled : public RuntimeError {
	public:
		RuntimeError(const wchar *name);
		STORM_CTOR RuntimeError(Str *msg);
		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Abstract function called: ");
			RuntimeError::message(to);
		}
	};

}
