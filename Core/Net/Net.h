#pragma once
#include "Core/Exception.h"
#include "OS/Handle.h"
#include "Core/Timing.h"

/**
 * Common includes for the network subsystem.
 */

#include "Utils/Platform.h"

#if defined(WINDOWS)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#elif defined(POSIX)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#else

#error "Please include socket headers for your platform."

#endif


namespace storm {
	STORM_PKG(core.net);

	/**
	 * Custom error type.
	 */
	class EXCEPTION_EXPORT NetError : public Exception {
		STORM_CLASS;
	public:
		NetError(const wchar *msg) {
			this->msg = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR NetError(Str *msg) {
			this->msg = msg;
			saveTrace();
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << S("Net error: ") << msg;
		}
	private:
		Str *msg;
	};

	// Initialize sockets (if needed).
	void initSockets();

	/**
	 * Helpers for socket manipulation.
	 */

	class Address;
	class Duration;

	// Create a socket.
	os::Handle createTcpSocket(int family);
	os::Handle createUdpSocket(int family);

	// Bind the socket.
	bool bindSocket(os::Handle socket, const sockaddr *addr, socklen_t addrSize);

	// Start listening from a socket.
	bool listenSocket(os::Handle socket, int backlog);

	// Accept a connection from a socket.
	os::Handle acceptSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr, socklen_t addrSize);

	// Connect.
	bool connectSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr, socklen_t addrSize);

	// Close a socket.
	void closeSocket(os::Handle socket, const os::Thread &attached);

	// Get the name of the socket (ie. local address bound to the socket).
	bool getSocketName(os::Handle socket, sockaddr *out, socklen_t size);

	// Get/setsockopt.
	bool getSocketOpt(os::Handle socket, int level, int name, void *value, socklen_t size);
	bool setSocketOpt(os::Handle socket, int level, int name, void *value, socklen_t size);

	Duration getSocketTime(os::Handle socket, int level, int name);
	bool setSocketTime(os::Handle socket, int level, int name, const Duration &d);

}
