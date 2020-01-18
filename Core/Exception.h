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


}
