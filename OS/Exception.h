#pragma once

namespace os {

	/**
	 * Errors in the threading.
	 *
	 * Note: Sadly, this can not be a Storm exception at the moment. We could solve it by having the
	 * threading runtime call an external function where needed.
	 */
	class ThreadError : public Exception {
	public:
		ThreadError(const String &msg) : msg(msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};


}
