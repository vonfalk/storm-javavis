#pragma once

namespace sound {

	/**
	 * Error.
	 */
	class EXCEPTION_EXPORT SoundOpenError : public Exception {
	public:
		SoundOpenError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Error while loading sound: " + msg; }
	private:
		String msg;
	};

}
