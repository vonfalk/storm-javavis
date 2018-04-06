#include "stdafx.h"
#include "Net.h"
#include "OS/IORequest.h"

namespace storm {

#if defined(WINDOWS)

	class WS {
	public:
		WS() {
			WSADATA data;
			if (WSAStartup(MAKEWORD(2, 2), &data)) {
				throw NetError(L"Unable to initialize sockets.");
			}
		}

		~WS() {
			WSACleanup();
		}
	};

	void initSockets() {
		static WS data;
	}

#else

	// No other system currently requires initialization to use sockets.
	void initSockets() {}

#endif


#if defined(WINDOWS)

	static os::Handle createSocket(int af, int mode, int proto) {
		SOCKET s = WSASocket(af, mode, proto, NULL, 0, WSA_FLAG_OVERLAPPED);

		u_long v = 1;
		ioctlsocket(s, FIONBIO, &v);

		return os::Handle((HANDLE)s);
	}

	static BOOL ConnectEx(os::Handle socket, const sockaddr *name, int namelen, OVERLAPPED *overlapped) {
		SOCKET s = (SOCKET)socket.v();
		LPFN_CONNECTEX ptr = 0;
		GUID guid = WSAID_CONNECTEX;
		DWORD bytes = 0;
		int ok = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, NULL, NULL);

		if (ok)
			throw NetError(L"Unable to acquire ConnectEx.");

		return (*ptr)(s, name, namelen, NULL, 0, NULL, overlapped);
	}

	bool connectSocket(os::Handle socket, const os::Thread &attached, sockaddr *addr) {
		// We need to bind the socket to use 'ConnectEx'...
		sockaddr_storage any;
		memset(&any, 0, sizeof(any));
		any.ss_family = addr->sa_family;
		if (bind((SOCKET)socket.v(), (sockaddr *)&any, sizeof(any))) {
			return false;
		}

		// TODO: Not always current!
		os::IORequest request(attached);

		if (ConnectEx(socket, addr, sizeof(sockaddr_storage), &request)) {
			// Already complete.
			return true;
		}

		int code = WSAGetLastError();
		if (code == ERROR_IO_PENDING) {
			// Wait for the result.
			request.wake.down();

			return request.error == 0;
		} else {
			// Failed in some other way.
			PLN(L"Failed: " << code);
			return false;
		}

		return false;
	}

	void closeSocket(os::Handle handle) {
		closesocket((SOCKET)handle.v());
	}

#elif defined(POSIX)

	void closeSocket(os::Handle handle) {
		close(handle.v());
	}

#else
#error "Please implement sockets for your platform!"
#endif

	os::Handle createTcpSocket(int family) {
		return createSocket(family, SOCK_STREAM, IPPROTO_TCP);
	}

	os::Handle createUdpSocket(int family) {
		return createSocket(family, SOCK_DGRAM, IPPROTO_UDP);
	}


}
