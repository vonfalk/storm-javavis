#pragma once

namespace storm {

	/**
	 * Generic exceptions.
	 */
	class EXCEPTION_EXPORT NotSupported : public Exception {
	public:
		NotSupported(const String &msg) : msg(L"Operation not supported: " + msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};


	/**
	 * Internal error.
	 */
	class EXCEPTION_EXPORT InternalError : public Exception {
	public:
		InternalError(const String &w) : w(w) {}
		virtual String what() const { return w; }
	private:
		String w;
	};


}
