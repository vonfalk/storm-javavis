#pragma once
#include "Utils/Exception.h"

/**
 * Common includes for the network subsystem.
 */

#include "Utils/Platform.h"

#if defined(WINDOWS)

#include <ws2tcpip.h>

#elif defined(POSIX)

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#else

#error "Please include socket headers for your platform."

#endif


namespace storm {

	/**
	 * Custom error type.
	 */
	class EXCEPTION_EXPORT NetError : public Exception {
	public:
		NetError(const String &msg) : msg(msg) {}
		virtual String what() const { return L"Net error: " + msg; }
	private:
		String msg;
	};

}
