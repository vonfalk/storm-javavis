#pragma once
#include "Utils/Exception.h"
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
	bool bindSocket(os::Handle socket, const sockaddr *addr, int addrSize);

	// Start listening from a socket.
	bool listenSocket(os::Handle socket, int backlog);

	// Accept a connection from a socket.
	os::Handle acceptSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr, int addrSize);

	// Connect.
	bool connectSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr);

	// Close a socket.
	void closeSocket(os::Handle socket);

	// Get the name of the socket (ie. local address bound to the socket).
	bool getSocketName(os::Handle socket, sockaddr *out, int size);

	// Get/setsockopt.
	bool getSocketOpt(os::Handle socket, int level, int name, void *value, socklen_t size);
	bool setSocketOpt(os::Handle socket, int level, int name, void *value, socklen_t size);

	Duration getSocketTime(os::Handle socket, int level, int name);
	bool setSocketTime(os::Handle socket, int level, int name, const Duration &d);

}
