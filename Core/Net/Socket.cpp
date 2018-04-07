#include "stdafx.h"
#include "Socket.h"
#include "Core/CloneEnv.h"

namespace storm {


	Socket::Socket(os::Handle handle) : handle(handle) {}

	Socket::~Socket() {
		if (handle)
			closeSocket(handle);
	}

	void Socket::close() {
		if (handle) {
			closeSocket(handle);
			handle = os::Handle();
		}
	}

	Duration Socket::inputTimeout() const {
		return getSocketTime(handle, SOL_SOCKET, SO_RCVTIMEO);
	}

	void Socket::inputTimeout(Duration v) {
		setSocketTime(handle, SOL_SOCKET, SO_RCVTIMEO, v);
	}

	Duration Socket::outputTimeout() const {
		return getSocketTime(handle, SOL_SOCKET, SO_SNDTIMEO);
	}

	void Socket::outputTimeout(Duration v) {
		setSocketTime(handle, SOL_SOCKET, SO_SNDTIMEO, v);
	}

	void Socket::toS(StrBuf *to) const {
		*to << S("Socket: ");
		if (handle) {
			sockaddr_storage data;
			memset(&data, 0, sizeof(data));
			if (getSocketName(handle, (sockaddr *)&data, sizeof(data))) {
				*to << toStorm(engine(), (sockaddr *)&data);
			} else {
				*to << S("<unknown>");
			}
		} else {
			*to << S("<closed>");
		}
	}

}
