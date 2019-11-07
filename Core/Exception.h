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


	/**
	 * Runtime errors.
	 */

	class EXCEPTION_EXPORT RuntimeError : public Exception {
	public:
		RuntimeError(const String &w) : w(w) {}
		virtual String what() const { return w; }
	private:
		String w;
	};


	/**
	 * Calling an abstract function.
	 */
	class EXCEPTION_EXPORT AbstractFnCalled : public RuntimeError {
	public:
		AbstractFnCalled(const String &name) : RuntimeError(L"Called an abstract function: " + name) {}
	};



}
