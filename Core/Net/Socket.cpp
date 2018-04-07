#include "stdafx.h"
#include "Socket.h"
#include "Core/CloneEnv.h"

namespace storm {


	Socket::Socket(os::Handle handle, os::Thread attachTo, Address *peer) : handle(handle), closed(0), peer(peer) {
		i = new (this) SocketIStream(this, attachTo);
		o = new (this) SocketOStream(this, attachTo);
	}

	Socket::~Socket() {
		if (handle)
			closeSocket(handle);
	}

	void Socket::close() {
		if (handle) {
			closeEnd(closeRead);
			closeEnd(closeWrite);
		}
	}

	void Socket::deepCopy(CloneEnv *env) {
		cloned(i, env);
		cloned(o, env);
	}

	void Socket::closeEnd(Nat which) {
		Nat old, w;
		do {
			old = atomicRead(closed);
			w = old | which;
		} while (atomicCAS(closed, old, w) != old);

		if (w == (closeRead | closeWrite) && handle) {
			closeSocket(handle);
			handle = os::Handle();
		}
	}

	SocketIStream *Socket::input() const {
		return i;
	}

	SocketOStream *Socket::output() const {
		return o;
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

	Bool Socket::nodelay() const {
		int v = 0;
		getSocketOpt(handle, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
		return v != 0;
	}

	void Socket::nodelay(Bool v) {
		int val = v ? 1 : 0;
		setSocketOpt(handle, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	}

	void Socket::toS(StrBuf *to) const {
		*to << S("Socket: ");
		if (handle) {
			sockaddr_storage data;
			if (getSocketName(handle, (sockaddr *)&data, sizeof(data))) {
				*to << toStorm(engine(), (sockaddr *)&data);
			} else {
				*to << S("<unknown>");
			}

			if (peer)
				*to << S(" <-> ") << peer;
		} else {
			*to << S("<closed>");
		}
	}


	/**
	 * Connect.
	 */

	MAYBE(Socket *) connect(Address *to) {
		initSockets();

		sockaddr_storage data;
		sockaddr *sa = (sockaddr *)&data;
		to->fill(sa);

		os::Handle h = createTcpSocket(sa->sa_family);
		os::Thread current = os::Thread::current();
		current.attach(h);

		try {
			// Connect!
			bool ok = connectSocket(h, current, sa);
			if (!ok)
				return false;

			return new (to) Socket(h, current, to);
		} catch (...) {
			closeSocket(h);
			throw;
		}
	}


	/**
	 * IStream.
	 */

	SocketIStream::SocketIStream(Socket *owner, os::Thread t) : HandleIStream(owner->handle, t) {}

	SocketIStream::~SocketIStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void SocketIStream::close() {
		owner->closeEnd(Socket::closeRead);
		handle = os::Handle();
	}


	/**
	 * OStream.
	 */

	SocketOStream::SocketOStream(Socket *owner, os::Thread t) : HandleOStream(owner->handle, t) {}

	SocketOStream::~SocketOStream() {
		// The socket will close the handle.
		handle = os::Handle();
	}

	void SocketOStream::close() {
		owner->closeEnd(Socket::closeWrite);
		handle = os::Handle();
	}


}
